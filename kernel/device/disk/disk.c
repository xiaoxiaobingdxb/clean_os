#include "disk.h"

#include "../../interrupt/intr.h"
#include "../../interrupt/intr_no.h"
#include "../../thread/schedule/segment.h"
#include "../device.h"
#include "common/cpu/contrl.h"
#include "common/lib/stdio.h"
#include "common/lib/string.h"
#include "common/types/std.h"

// @see https://wiki.osdev.org/ATA_PIO_Mode
#define PRIMARY_PORT_BASE 0x1F0
enum {
    PORT_DATA = 0,
    PORT_ERR,
    PORT_SECTOR_COUNT,
    PORT_SECTOR_LOW,
    PORT_SECTOR_MIDDLE,
    PORT_SECTOR_HIGH,
    PORT_DRIVE,
    PORT_STATUS_COMMAND,
};
#define disk_port(disk, port) (disk->port_base + port)
#define disk_drive(disk) ((disk->drive << 4) | 0xE0)  // 0b11100000)

enum {
    STATUS_ERR = 0,  // status error
    STATUS_IDX,
    STATUS_CORR,
    STATUS_DRQ,  // ready to read data
    STATUS_SRV,
    STATUS_DF,
    STATUS_RDY,
    STATUS_BSY,  // disk is busy
} disk_status;
#define disk_status(status) (1 << status)

#define COMMAND_IDENTIFY 0xec
#define COMMAND_READ 0x24
#define COMMAND_WRITE 0x34

#define DISK_COUNT 2
disk_t disks[DISK_COUNT];

void ata_cmd(disk_t *disk, uint32_t sector_start, uint32_t sector_count,
             uint8_t cmd) {
    outb(disk_port(disk, PORT_DRIVE), disk_drive(disk));  // set drive

    outb(disk_port(disk, PORT_SECTOR_COUNT),
         (uint8_t)(sector_count >> 8));  // sector_count:8-head
    outb(disk_port(disk, PORT_SECTOR_LOW),
         (uint8_t)(sector_start >> 24));  // sector_start:24-31
    outb(disk_port(disk, PORT_SECTOR_MIDDLE), 0);
    outb(disk_port(disk, PORT_SECTOR_HIGH), 0);

    outb(disk_port(disk, PORT_SECTOR_COUNT),
         (uint8_t)sector_count);  // sector_count:0-7
    outb(disk_port(disk, PORT_SECTOR_LOW),
         (uint8_t)sector_start);  // sector_start:0-7
    outb(disk_port(disk, PORT_SECTOR_MIDDLE),
         (uint8_t)(sector_start >> 8));  // sector_start:8-15
    outb(disk_port(disk, PORT_SECTOR_HIGH),
         (uint8_t)(sector_start >> 16));  // sector_start:16-23

    // put cmd
    outb(disk_port(disk, PORT_STATUS_COMMAND), cmd);
}

error ata_wait(disk_t *disk) {
    uint8_t status;
    while (true) {
        status = inb(disk_port(disk, PORT_STATUS_COMMAND));
        if ((status & (disk_status(STATUS_ERR) | disk_status(STATUS_DRQ) |
                       disk_status(STATUS_BSY))) != disk_status(STATUS_BSY)) {
            break;
        }
    }
    return status & disk_status(STATUS_ERR) == disk_status(STATUS_ERR);
}

ssize_t ata_read(disk_t *disk, const byte_t *buf, size_t size) {
    uint16_t *data = (uint16_t *)buf;
    for (int i = 0; i < size / 2; i++) {
        *data++ = inw(disk_port(disk, PORT_DATA));
    }
    return size;
}

ssize_t ata_write(disk_t *disk, const byte_t *buf, size_t size) {
    uint16_t *data = (uint16_t *)buf;
    for (int i = 0; i < size / 2; i++) {
        outw(disk_port(disk, PORT_DATA), *(data + i));
    }
    return size;
}

#pragma pack(1)
// @see https://wiki.osdev.org/Partition_Table
typedef struct {
    uint8_t is_active;
    uint8_t head_start;
    uint8_t sector_start : 6;
    uint16_t cylinder_start : 10;
    uint8_t system_id;
    uint8_t head_end;
    uint8_t sector_end : 6;
    uint16_t cylinder_end : 10;
    uint32_t relative_sector;
    uint32_t sector_total;
} part_item_t;

typedef struct {
    uint8_t mbr_code[446];
    part_item_t part_items[PART_COUNT];
    uint8_t mbr_sig[2];
} mbr_t;
#pragma pack()

error detect_parts(disk_t *disk) {
    ata_cmd(disk, 0, 1, COMMAND_READ);
    error err = ata_wait(disk);
    if (err) {
        return err;
    }
    mbr_t mbr;
    ssize_t size = ata_read(disk, (byte_t *)&mbr, sizeof(mbr));
    if (size < 0) {
        return size;
    }

    for (int i = 0; i < PART_COUNT; i++) {
        part_info_t *part = disk->parts + i + 1;
        part_item_t *item = mbr.part_items + i;
        part->type = item->system_id;
        if (part->type == PART_UNVALID) {
            memset(part->name, 0, sizeof(part->name));
            part->sector_start = 0;
            part->sector_count = 0;
            part->disk = NULL;
        } else {
            sprintf(part->name, "%s%d", disk->name, i);
            part->sector_start = item->relative_sector;
            part->sector_count = item->sector_total;
            part->disk = disk;
        }
    }
    return 0;
}

/**
 * @brief
 * @return 0 if no error otherwise not 0
 */
error detect_disk(disk_t *disk) {
    ata_cmd(disk, 0, 0, COMMAND_IDENTIFY);
    if (inb(disk_port(disk, PORT_STATUS_COMMAND)) ==
        0) {  // status register not found
        return -1;
    }
    error err = ata_wait(disk);
    if (err) {
        return true;
    }
    uint16_t buf[256];
    memset(buf, 0, sizeof(buf));
    int size = ata_read(disk, (uint8_t *)buf, sizeof(buf));
    if (size < 0) {
        return size;
    }
    disk->sector_count = *((uint32_t *)(buf + 100));
    disk->sector_size = SECTOR_SIZE;

    part_info_t *part0 = disk->parts + 0;
    sprintf(part0->name, "%s%d", disk->name, 0);
    part0->disk = disk;
    part0->sector_start = 0;
    part0->sector_count = disk->sector_count;
    detect_parts(disk);
    return 0;
}

segment_t segment;
void disk_hander(uint32_t intr_no) { 
    segment_wakeup(&segment, NULL);
}

void init_disk() {
    for (int i = 0; i < DISK_COUNT; i++) {
        disk_t *disk = disks + i;
        sprintf(disk->name, "sd%c", 'a' + i);
        disk->port_base = PRIMARY_PORT_BASE;
        disk->drive = i;
        detect_disk(disk);
    }

    init_segment(&segment, 0);
    register_intr_handler(INTR_NO_DISK, disk_hander);
}

error disk_open(device_t *dev) {
    int disk_idx = dev->minor >> 4;
    int part_idx = dev->minor & 0xf;
    if (disk_idx >= DISK_COUNT || part_idx >= PART_COUNT) {
        return -1;
    }
    disk_t *disk = disks + disk_idx;
    if (disk->sector_size == 0) {
        return -2;
    }
    part_info_t *part = disk->parts + part_idx;
    if (part->sector_count == 0) {
        return -3;
    }
    dev->data = part;
    return 0;
}

ssize_t disk_write(device_t *dev, uint32_t addr, const byte_t *buf,
                   size_t size) {
    part_info_t *part = (part_info_t *)dev->data;
    if (part == NULL) {
        return -1;
    }
    disk_t *disk = NULL;
    if ((disk = part->disk) == NULL) {
        return -2;
    }
    ata_cmd(disk, addr + part->sector_start, size, COMMAND_WRITE);
    ssize_t write_size = -1;
    for (int i = 0; i < size; i++, buf += disk->sector_size * i) {
        if ((write_size = ata_write(disk, buf, disk->sector_size)) <= 0) {
            return write_size;
        }
        
        segment_wait(&segment, NULL);
        error err = ata_wait(disk);
        if (err) {
            return err;
        }
    }
    return size;
}
ssize_t disk_read(device_t *dev, uint32_t addr, byte_t *buf, size_t size) {
    part_info_t *part = (part_info_t *)dev->data;
    if (part == NULL) {
        return -1;
    }
    disk_t *disk = NULL;
    if ((disk = part->disk) == NULL) {
        return -2;
    }
    ata_cmd(disk, addr + part->sector_start, size, COMMAND_READ);
    for (int i = 0; i < size; i++, buf += disk->sector_size * i) {
        segment_wait(&segment, NULL);
        error err = ata_wait(disk);
        if (err) {
            return err;
        }
        ssize_t read_size = -1;
        if ((read_size = ata_read(disk, buf, disk->sector_size)) <= 0) {
            return read_size;
        }
    }
    return size;
}
int disk_control(device_t *dev, int, int, int) { return 0; }
void disk_close(device_t *dev) {}

device_desc_t dev_disk_desc = {
    .name = "disk",
    .major = DEV_DISK,
    .open = disk_open,
    .read = disk_read,
    .write = disk_write,
    .control = disk_control,
    .close = disk_close,
};
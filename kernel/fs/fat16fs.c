
#include "../device/disk/disk.h"
#include "../memory/mem.h"
#include "common/lib/string.h"
#include "fs.h"
typedef uint16_t fat16_cluster_t;
#define INVALID_CLUSTER 0xfff8
#pragma pack(1)
/**
 * @see
 * https://wiki.osdev.org/FAT#:~:text=Boot%20Record.-,BPB%20(BIOS%20Parameter%20Block),-The%20boot%20record
 */
typedef struct {
    uint8_t jump[3];
    uint8_t oem[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sector_count;
    uint8_t fat_count;
    uint16_t root_entry_count;
    uint16_t sector_count;
    uint8_t media_type;
    uint16_t sectors_per_fat;
    uint16_t sectors_per_track;
    uint16_t head_count;
    uint32_t hidden_sector_count;
    uint32_t large_sector_count;

    uint8_t drive_number;
    uint8_t flags;
    uint8_t signature;
    uint32_t volume_id;
    uint8_t volume_lable[11];
    uint8_t file_system[8];

    size_t cache_sector_start;
    size_t cache_sector_size;
    void *cache_buf;
} bpb_t;

#define fat16_bytes_per_cluster(bpb) \
    (bpb->bytes_per_sector * bpb->sectors_per_cluster)
#define fat16_tabl1_sector_idx(bpb) (bpb->reserved_sector_count)
#define fat16_root_sector_idx(bpb) \
    (fat16_tabl1_sector_idx(bpb) + bpb->fat_count * bpb->sectors_per_fat)
#define fat16_data_sector_idx(bpb) (fat16_root_sector_idx(bpb) + bpb->root_entry_count * sizeof(dir_entry_t) / bpb->bytes_per_sector)
#define sector_idx_to_addr(bpb, idx) (idx * bpb->bytes_per_sector)

#define DIR_ENTRY_ATTR_DIR 0x10
/**
 * @see
 * https://wiki.osdev.org/FAT#:~:text=entries%20for%20that.-,Directories%20on%20FAT12/16/32,-A%20directory%20entry
 */
typedef struct {
    uint8_t name[8];
    uint8_t ext[3];
    uint8_t attr;
    uint8_t reserved;
    uint8_t create_time_ms;
    uint16_t create_time;
    uint16_t create_date;
    uint16_t access_data;
    uint16_t cluster_high;
    uint16_t write_time;
    uint16_t write_date;
    uint16_t cluster_low;
    uint32_t file_size;
} dir_entry_t;

typedef struct {
    uint8_t order;
    uint8_t name0_4[10];
    uint8_t attr;
    uint8_t type;
    uint8_t check_sum;
    uint8_t name5_10[12];
    uint16_t reserved;
    uint8_t name11_12[4];
} long_name_dir_entry_t;
#pragma pack()

#define DIR_END 0x0
#define DIR_FREE 0xE5
#define DIR_LONG_NAME 0x0F

bool validate_bpb(bpb_t *bpb) {
    if (bpb->fat_count != 2) {
        return false;
    }
    if (memcmp(bpb->file_system, "FAT16", 5)) {
        return false;
    }
    return true;
}

error fat16_mount(fs_desc_t *fs, int major, int minor) {
    dev_id_t dev_id = device_open(major, minor, NULL);
    if (dev_id < 0) {
        goto mount_fail;
    }

    uint8_t *bpb_buf = (uint8_t *)alloc_kernel_mem(1);
    if (!bpb_buf) {
        goto mount_fail;
    }
    if (device_read(dev_id, 0, (byte_t *)bpb_buf, 1) < 1) {
        goto mount_fail;
    }
    bpb_t *bpb = (bpb_t *)bpb_buf;
    if (!validate_bpb(bpb)) {
        goto mount_fail;
    }

    bpb->cache_buf = (void *)(&(bpb->cache_buf) + sizeof(bpb->cache_buf));

    fs->type = FS_FAT16;
    fs->data = bpb;
    fs->dev_id = dev_id;
    return 0;
mount_fail:
    if (dev_id >= 0) {
        device_close(dev_id);
    }
    if (bpb_buf) {
        unalloc_kernel_mem((uint32_t)bpb_buf, 1);
    }
    return -1;
}
void fat16_unmount(fs_desc_t *fs) {
    if (!fs) {
        return;
    }
    device_close(fs->dev_id);
    bpb_t *bpb = fs->data;
    if (bpb) {
        unalloc_kernel_mem((uint32_t)bpb, 1);
    }
}

// read a sector data from disk
error buf_read(dev_id_t dev_id, bpb_t *bpb, size_t sector_start, size_t size) {
    if (bpb->cache_sector_start == sector_start &&
        bpb->cache_sector_size == size) {
        // use cache
        return 0;
    }
    if (device_read(dev_id, sector_start, bpb->cache_buf, size) > 0) {
        bpb->cache_sector_start = sector_start;
        bpb->cache_sector_size = size;
        return 0;
    }
    return -1;
}
dir_entry_t *read_dir_entry(fs_desc_t *fs, bpb_t *bpb, int idx) {
    if (idx < 0 || idx > bpb->root_entry_count) {
        return NULL;
    }
    size_t offset = sizeof(dir_entry_t) * idx;
    size_t sector_start =
        offset / bpb->bytes_per_sector + fat16_root_sector_idx(bpb);
    if (buf_read(fs->dev_id, bpb, sector_start, 1)) {
        return NULL;
    }
    dir_entry_t *entry =
        (dir_entry_t *)(bpb->cache_buf + offset % bpb->bytes_per_sector);
    return entry;
}

#define SFN_LEN 11
/**
 * @details fat16/32 8.3
 */
void to_sfn(char *buf, const char *path) {
    memset(buf, ' ', SFN_LEN);
    for (int i = 0, j = 0; i < strlen(path); i++, j++) {
        char c = path[i];
        if (c == '.') {
            j = 7;
            continue;
        }
        buf[j] = c - 'a' + 'A';
    }
}
bool match_file_name(dir_entry_t *dir, const char *path) {
    char sfn[SFN_LEN];
    to_sfn(sfn, path);
    bool match = !memcmp(dir->name, sfn, SFN_LEN);
    return match;
}
error fat16_open(fs_desc_t *fs, const char *path, file_t *file) {
    bpb_t *bpb = (bpb_t *)fs->data;
    if (!bpb) {
        return -1;
    }
    int file_idx = -1;
    dir_entry_t *item = NULL;
    for (int i = 0; i < bpb->root_entry_count; i++) {
        dir_entry_t *dir = read_dir_entry(fs, bpb, i);
        if (dir->name[0] == DIR_END) {
            break;
        }
        if (dir->name[0] == DIR_FREE) {
            continue;
        }

        if (match_file_name(dir, path)) {
            file_idx = i;
            item = dir;
            break;
        }
    }
    if (item) {
        file_type type = FILE_FILE;
        if (item->attr & DIR_ENTRY_ATTR_DIR) {
            type = FILE_DIR;
        }

        file->size = item->file_size;

        file->pos = 0;
        file->type = type;
        file->block_cur = file->block_cur =
            item->cluster_high >> 16 | (item->cluster_low - 2);
    }
    return 0;
}

fat16_cluster_t get_next_cluster(dev_id_t dev_id, bpb_t *bpb, fat16_cluster_t cur) {
    uint32_t offset = cur * sizeof(fat16_cluster_t);
    uint32_t sector_offset = offset % bpb->bytes_per_sector;
    uint32_t sector_idx = offset / bpb->bytes_per_sector;
    if (sector_idx > bpb->sectors_per_fat) {
        return INVALID_CLUSTER;
    }
    error err = buf_read(dev_id, bpb, bpb->reserved_sector_count + sector_idx, 1);
    if (err) {
        return INVALID_CLUSTER;
    }
    return *(fat16_cluster_t*)bpb->cache_buf + sector_offset;
}
ssize_t fat16_read(file_t *file, byte_t *buf, size_t size) {
    bpb_t *bpb = (bpb_t *)file->desc->data;
    if (!bpb) {
        return -1;
    }
    ssize_t total_size = 0;
    if (file->pos + size > file->size) {
        size = file->size - file->pos;
    }
    while (size > 0) {
        uint32_t step_read_size = size;
        uint32_t cluster_offset = file->pos % fat16_bytes_per_cluster(bpb);
        uint32_t sector_start = fat16_data_sector_idx(bpb) +
                                file->block_cur * bpb->sectors_per_cluster;
        error err = buf_read(file->desc->dev_id, bpb, sector_start,
                             bpb->sectors_per_cluster);
        if (err) {
            return total_size;
        }
        if (step_read_size + cluster_offset > fat16_bytes_per_cluster(bpb)) {
            step_read_size = fat16_bytes_per_cluster(bpb) - cluster_offset;
        }
        memcpy(buf, bpb->cache_buf + cluster_offset, step_read_size);
        buf += step_read_size;
        size -= step_read_size;
        total_size += step_read_size;
        file->pos += step_read_size;
        if (step_read_size + cluster_offset >= fat16_bytes_per_cluster(bpb)) {
            file->block_cur = get_next_cluster(file->desc->dev_id, bpb, file->block_cur);
        }
    }
    return total_size;
}
ssize_t fat16_write(file_t *file, const byte_t *buf, size_t size) {}
void fat16_close(file_t *file) {}
error fat16_seek(file_t *file, size_t offset) {}
error fat16_ioctl(file_t *file, int cmd, int arg0, int arg1) {}

fs_ops_t fat16_ops = {
    .mount = fat16_mount,
    .unmount = fat16_unmount,
    .open = fat16_open,
    .read = fat16_read,
    .write = fat16_write,
    .close = fat16_close,
    .seek = fat16_seek,
    .ioctl = fat16_ioctl,
};
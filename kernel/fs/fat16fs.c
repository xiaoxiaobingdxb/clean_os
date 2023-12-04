
#include "../device/disk/disk.h"
#include "../memory/mem.h"
#include "../syscall/syscall_user.h"
#include "common/lib/string.h"
#include "common/tool/datetime.h"
#include "common/tool/math.h"
#include "fs.h"

typedef uint16_t fat16_cluster_t;
#define INVALID_CLUSTER 0xfff8
#define FREE_CLUSTER 0x0
#define CLUSTER_START 0x2
#define is_valid_cluster(cluster) \
    (cluster >= CLUSTER_START && cluster < INVALID_CLUSTER)
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
#define fat16_data_sector_idx(bpb) \
    (fat16_root_sector_idx(bpb) +  \
     bpb->root_entry_count * sizeof(dir_entry_t) / bpb->bytes_per_sector)
#define sector_idx_to_addr(bpb, idx) (idx * bpb->bytes_per_sector)

#define DIR_ENTRY_ATTR_DIR 0x10

/**
 * @see
 * https://wiki.osdev.org/FAT#:~:text=entries%20for%20that.-,Directories%20on%20FAT12/16/32,-A%20directory%20entry
 */
typedef union {
    struct {
        uint8_t hour : 5;
        uint8_t minu : 6;
        uint8_t second : 5;
    };
    uint16_t v;
} file_time;

typedef union {
    struct {
        uint8_t year : 7;
        uint8_t month : 4;
        uint8_t day : 5;
    };
    uint16_t v;
} file_date;

#define YEAR_OFFSET 1900
void datetime2timespec(file_date *date, file_time *time, timespec_t *timespec) {
    time64_t timeSec = mktime64(date->year + YEAR_OFFSET, date->month, date->day, time->hour,
                                time->minu, time->second);
    timespec->tv_sec = timeSec;
}

typedef struct {
    uint8_t name[8];
    uint8_t ext[3];
    uint8_t attr;
    uint8_t reserved;
    uint8_t create_time_ms;
    file_time create_time;
    file_date create_date;
    file_date access_date;
    uint16_t cluster_high;
    file_time write_time;
    file_date write_date;
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

error buf_write(dev_id_t dev_id, bpb_t *bpb, size_t sector_start, size_t size) {
    if (device_write(dev_id, sector_start, bpb->cache_buf, size) > 0) {
        return 0;
    }
    return -1;
}

dir_entry_t *read_dir_entry(dev_id_t dev_id, bpb_t *bpb, int idx) {
    if (idx < 0 || idx >= bpb->root_entry_count) {
        return NULL;
    }
    size_t offset = sizeof(dir_entry_t) * idx;
    size_t sector_start =
        offset / bpb->bytes_per_sector + fat16_root_sector_idx(bpb);
    if (buf_read(dev_id, bpb, sector_start, 1)) {
        return NULL;
    }
    dir_entry_t *entry =
        (dir_entry_t *)(bpb->cache_buf + offset % bpb->bytes_per_sector);
    return entry;
}

error write_dir_entry(dev_id_t dev_id, bpb_t *bpb, dir_entry_t *entry,
                      int idx) {
    if (idx < 0 || idx >= bpb->root_entry_count) {
        return -1;
    }
    size_t offset = sizeof(dir_entry_t) * idx;
    size_t sector_start =
        offset / bpb->bytes_per_sector + fat16_root_sector_idx(bpb);
    error err;
    if ((err = buf_read(dev_id, bpb, sector_start, 1)) != 0) {
        return err;
    }
    uint32_t sector_offset = offset % bpb->bytes_per_sector;
    memcpy(bpb->cache_buf + sector_offset, entry, sizeof(dir_entry_t));
    if ((err = buf_write(dev_id, bpb, sector_start, 1)) != 0) {
        return err;
    }
    return 0;
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
bool match_file_name(const char *entry_name, const char *path) {
    char sfn[SFN_LEN];
    to_sfn(sfn, path);
    bool match = !memcmp(entry_name, sfn, SFN_LEN);
    return match;
}

error set_cluster(dev_id_t dev_id, bpb_t *bpb, fat16_cluster_t cur,
                  fat16_cluster_t next) {
    if (!is_valid_cluster(cur)) {
        return -1;
    }
    uint32_t offset = cur * sizeof(fat16_cluster_t);
    uint32_t sector_idx = offset / bpb->bytes_per_sector;
    if (sector_idx >= bpb->sectors_per_fat) {
        return -2;
    }
    uint32_t sector_start = fat16_tabl1_sector_idx(bpb) + sector_idx;
    error err = buf_read(dev_id, bpb, sector_start, 1);
    if (err) {
        return err;
    }
    uint32_t sector_offset = offset % bpb->bytes_per_sector;
    *(fat16_cluster_t *)(bpb->cache_buf + sector_offset) = next;
    for (int i = 0; i < bpb->fat_count; i++) {
        buf_write(dev_id, bpb, sector_start + i * bpb->sectors_per_fat, 1);
    }
    return 0;
}

void read_file_info(file_t *file, dir_entry_t *entry, uint32_t file_idx) {
    file_type type = FILE_FILE;
    if (entry->attr & DIR_ENTRY_ATTR_DIR) {
        type = FILE_DIR;
    }

    file->file_idx = file_idx;
    file->size = entry->file_size;

    file->pos = 0;
    file->type = type;
    file->block_start = file->block_cur =
        entry->cluster_high >> 16 | entry->cluster_low;
}

#include "../device/rtc/cmos.h"
void init_dir_entry(dir_entry_t *entry, const char *name, uint8_t attr) {
    to_sfn(entry->name, name);
    entry->attr = attr;
    entry->reserved = 0;
    entry->create_time_ms = 0;
    time_t time;
    get_rtc_time(&time);
    file_time ftime = {
        .hour = time.hour, .minu = time.minute, .second = time.second};
    file_date date = {.year = time.year, .month = time.month, .day = time.day};
    entry->create_time = ftime;
    entry->create_date = date;
    entry->access_date = date;
    entry->cluster_high = INVALID_CLUSTER >> 16;
    entry->write_time = ftime;
    entry->write_date = date;
    entry->cluster_low = INVALID_CLUSTER;
    entry->file_size = 0;
}

fat16_cluster_t get_next_cluster(dev_id_t dev_id, bpb_t *bpb,
                                 fat16_cluster_t cur) {
    uint32_t offset = cur * sizeof(fat16_cluster_t);
    uint32_t sector_offset = offset % bpb->bytes_per_sector;
    uint32_t sector_idx = offset / bpb->bytes_per_sector;
    if (sector_idx > bpb->sectors_per_fat) {
        return INVALID_CLUSTER;
    }
    error err =
        buf_read(dev_id, bpb, bpb->reserved_sector_count + sector_idx, 1);
    if (err) {
        return INVALID_CLUSTER;
    }
    return *(fat16_cluster_t *)bpb->cache_buf + sector_offset;
}

void free_cluster_chain(dev_id_t dev_id, bpb_t *bpb, fat16_cluster_t cluster) {
    while (is_valid_cluster(cluster)) {
        fat16_cluster_t next = get_next_cluster(dev_id, bpb, cluster);
        set_cluster(dev_id, bpb, cluster, next);
        cluster = next;
    }
}

error fat16_open(fs_desc_t *fs, const char *path, file_t *file) {
    bpb_t *bpb = (bpb_t *)fs->data;
    if (!bpb) {
        return -1;
    }
    int file_idx = -1;
    dir_entry_t *item = NULL;
    for (int i = 0; i < bpb->root_entry_count; i++) {
        dir_entry_t *entry = read_dir_entry(fs->dev_id, bpb, i);
        if (!entry) {
            return -2;
        }
        file_idx = i;
        if (entry->name[0] == DIR_END) {
            break;
        }
        if (entry->name[0] == DIR_FREE) {
            continue;
        }

        if (match_file_name(entry->name, path)) {
            item = entry;
            break;
        }
    }
    if (item) {
        read_file_info(file, item, file_idx);
        if (file->mode & O_TRUNC) {
            free_cluster_chain(file->desc->dev_id, bpb, file->block_start);
            file->block_start = file->block_cur = INVALID_CLUSTER;
            file->size = 0;
        }
        return 0;
    } else if ((file->mode | O_CREAT) &&
               file_idx >= 0) {  // create file with dir_entry
        dir_entry_t entry;
        init_dir_entry(&entry, path, 0);
        error err;
        if ((err = write_dir_entry(file->desc->dev_id, bpb, &entry,
                                   file_idx)) != 0) {
            return err;
        }
        read_file_info(file, &entry, file_idx);
    }

    return 0;
}
typedef enum {
    fat16_file_operate_read,
    fat16_file_operate_write
} fat16_file_operate_t;
ssize_t fat16_file_operate(file_t *file, byte_t *buf, size_t size,
                           fat16_file_operate_t operate) {
    bpb_t *bpb = (bpb_t *)file->desc->data;
    if (!bpb) {
        return -1;
    }
    ssize_t total_size = 0;
    if (operate == fat16_file_operate_read && file->pos + size > file->size) {
        size = file->size - file->pos;
    }
    while (size > 0) {
        uint32_t step_read_size = size;
        uint32_t cluster_offset = file->pos % fat16_bytes_per_cluster(bpb);
        uint32_t sector_start =
            fat16_data_sector_idx(bpb) +
            (file->block_cur - CLUSTER_START) * bpb->sectors_per_cluster;
        error err;
        if ((err = buf_read(file->desc->dev_id, bpb, sector_start,
                            bpb->sectors_per_cluster)) != 0) {
            return total_size;
        }
        void *copyDst, *copySrc;
        if (operate == fat16_file_operate_read) {
            copyDst = buf;
            copySrc = bpb->cache_buf + cluster_offset;
        } else if (operate == fat16_file_operate_write) {
            copyDst = bpb->cache_buf + cluster_offset;
            copySrc = buf;
        } else {
            return -1;
        }
        if (step_read_size + cluster_offset > fat16_bytes_per_cluster(bpb)) {
            step_read_size = fat16_bytes_per_cluster(bpb) - cluster_offset;
        }
        memcpy(copyDst, copySrc, step_read_size);
        if (operate == fat16_file_operate_write &&
            (err = buf_write(file->desc->dev_id, bpb, sector_start,
                             bpb->sectors_per_cluster)) != 0) {
            return total_size;
        }
        buf += step_read_size;
        size -= step_read_size;
        total_size += step_read_size;
        file->pos += step_read_size;
        if (step_read_size + cluster_offset >= fat16_bytes_per_cluster(bpb)) {
            file->block_cur =
                get_next_cluster(file->desc->dev_id, bpb, file->block_cur);
        }
    }
    return total_size;
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
        uint32_t sector_start =
            fat16_data_sector_idx(bpb) +
            (file->block_cur - CLUSTER_START) * bpb->sectors_per_cluster;
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
            file->block_cur =
                get_next_cluster(file->desc->dev_id, bpb, file->block_cur);
        }
    }
    return total_size;
}

fat16_cluster_t malloc_free_cluster(dev_id_t dev_id, bpb_t *bpb, int count) {
    fat16_cluster_t start = INVALID_CLUSTER, pre = INVALID_CLUSTER;
    size_t cluster_total_count =
        bpb->sectors_per_fat * bpb->bytes_per_sector / sizeof(fat16_cluster_t);
    for (fat16_cluster_t cur = CLUSTER_START;
         cur < cluster_total_count, count > 0; cur++) {
        if (!is_valid_cluster(start)) {
            start = cur;
        }
        if (is_valid_cluster(pre) && set_cluster(dev_id, bpb, pre, cur) != 0) {
            free_cluster_chain(dev_id, bpb, start);
            return INVALID_CLUSTER;
        }
        pre = cur;
        count--;
    }
    if (count == 0 && set_cluster(dev_id, bpb, pre, INVALID_CLUSTER) ==
                          0) {  // alloc all required cluster
        return start;
    }
    free_cluster_chain(dev_id, bpb, start);
    return INVALID_CLUSTER;
}

error expand_file(file_t *file, bpb_t *bpb, size_t size) {
    size_t bytes_per_cluster = fat16_bytes_per_cluster(bpb);
    size_t cluster_count = 0;
    if (file->size == 0 || file->size % bytes_per_cluster == 0) {
        cluster_count = up2(size, bytes_per_cluster) / bytes_per_cluster;
    } else {
        // file disk space alloc by a cluster, so file realy freely size may be
        // bigger than file' size
        size_t file_free = bytes_per_cluster - file->size % bytes_per_cluster;
        if (file_free >= size) {
            return 0;
        }
        cluster_count =
            up2(size - file_free, bytes_per_cluster) / bytes_per_cluster;
    }
    fat16_cluster_t start =
        malloc_free_cluster(file->desc->dev_id, bpb, cluster_count);
    if (!is_valid_cluster(start)) {
        return -2;
    }
    if (!is_valid_cluster(file->block_start)) {
        file->block_start = file->block_cur = start;
    } else {
        set_cluster(file->desc->dev_id, bpb, file->block_cur, start);
    }
    return 0;
}
ssize_t fat16_write(file_t *file, const byte_t *buf, size_t size) {
    bpb_t *bpb = (bpb_t *)file->desc->data;
    if (!bpb) {
        return -1;
    }

    if (file->pos + size > file->size && expand_file(file, bpb, size) != 0) {
        return -1;
    }
    file->size = file->pos;
    ssize_t total_size =
        fat16_file_operate(file, (byte_t *)buf, size, fat16_file_operate_write);
    if (total_size > 0) {
        file->size += total_size;
    }
    return total_size;
}

void fat16_close(file_t *file) {
    if (file->mode & O_RDONLY) {
        return;
    }
    bpb_t *bpb = (bpb_t *)file->desc->data;
    if (!bpb) {
        return;
    }
    dir_entry_t *entry =
        read_dir_entry(file->desc->dev_id, bpb, file->file_idx);
    if (!entry) {
        return;
    }
    time_t time;
    get_rtc_time(&time);
    file_time ftime = {
        .hour = time.hour, .minu = time.minute, .second = time.second};
    file_date date = {.year = time.year - YEAR_OFFSET, .month = time.month, .day = time.day};
    entry->access_date.v = date.v;
    entry->write_time.v = ftime.v;
    entry->write_date.v = date.v;
    entry->cluster_high = file->block_start >> 16;
    entry->cluster_low = file->block_start;
    entry->file_size = file->size;
    write_dir_entry(file->desc->dev_id, bpb, entry, file->file_idx);
}
off_t fat16_seek(file_t *file, off_t offset, int whence) {
    off_t ret_offset = -1;
    switch (whence) {
    case SEEK_SET:
        offset = max(offset, 0);
        ret_offset = file->pos;
        file->pos = offset;
        ret_offset = file->pos - ret_offset;
        break;
    case SEEK_CUR:
        uint64_t pos = file->pos + offset;
        if (pos >= 1 << sizeof(file->pos)) {
            ret_offset = EOVERFLOW;
        } else {
            ret_offset = file->pos;
            file->pos += offset;
            ret_offset = file->pos - ret_offset;
        }
        break;
    case SEEK_END:
        ret_offset = file->pos;
        file->pos = max(file->size + offset, 0);
        ret_offset = file->pos - ret_offset;
    default:
        ret_offset = EINVAL;
        break;
    }
    return ret_offset;
}

int get_file_cluster_count(file_t *file) {
    bpb_t *bpb = (bpb_t *)file->desc->data;
    if (!bpb) {
        return -1;
    }
    int cluster_count = 0;
    fat16_cluster_t cur = file->block_start;
    while (cur != INVALID_CLUSTER) {
        cluster_count++;
        cur = get_next_cluster(file->desc->dev_id, bpb, cur);
    }
    return cluster_count;
}

error fat16_stat(file_t *file, void *data) {
    if (!data) {
        return 0;
    }
    stat_t *stat = (stat_t *)data;
    stat->dev_id = file->desc->dev_id;
    stat->size = file->size;
    bpb_t *bpb = (bpb_t *)file->desc->data;
    if (!bpb) {
        return -1;
    }
    stat->block_size = bpb->bytes_per_sector;
    stat->block_count =
        fat16_bytes_per_cluster(bpb) * get_file_cluster_count(file);
    dir_entry_t *entry =
        read_dir_entry(file->desc->dev_id, bpb, file->file_idx);
    if (!entry) {
        return 0;
    }
    datetime2timespec(&entry->create_date, &entry->create_time,
                      &stat->create_time);
    stat->create_time.tv_nsec = ms2ns(entry->create_time_ms);
    datetime2timespec(&entry->write_date, &entry->write_time,
                      &stat->update_time);
    return 0;
}
error fat16_ioctl(file_t *file, int cmd, int arg0, int arg1) { return -1; }

fs_ops_t fat16_ops = {
    .mount = fat16_mount,
    .unmount = fat16_unmount,
    .open = fat16_open,
    .read = fat16_read,
    .write = fat16_write,
    .close = fat16_close,
    .seek = fat16_seek,
    .stat = fat16_stat,
    .ioctl = fat16_ioctl,
};
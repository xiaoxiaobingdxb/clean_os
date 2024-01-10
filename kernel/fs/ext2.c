#include "../device/disk/disk.h"
#include "../memory/malloc/malloc.h"
#include "common/lib/string.h"
#include "common/tool/log.h"
#include "common/tool/math.h"
#include "common/types/byte.h"
#include "fs.h"

/**
 * @see https://wiki.osdev.org/Ext2
 */
#define kb_size(s) ((s + 1) * KB(1))
#define has_super_block(i) \
    (i == 0 || is_pow(i, 3) || is_pow(i, 5) || is_pow(i, 7))
#pragma pack(1)
typedef struct {
    uint32_t inode_total;
    uint32_t blocks_total;
    uint32_t blocks_reserved_count;
    uint32_t unallocated_block_count;
    uint32_t unallocated_inode_count;
    uint32_t super_block_id;
    uint32_t block_size_kb;
    uint32_t fragment_size_kb;
    uint32_t blocks_per_group;
    uint32_t fragments_per_group;
    uint32_t inodes_per_group;
    uint32_t last_mount_time;
    uint32_t last_written_time;
    uint16_t mounts_since_last_check;
    uint16_t max_mounts_since_last_check;
    uint16_t ext2_sig;  // 0xef53
    uint16_t state;
    uint16_t error_handling_method;
    uint16_t minor_version;
    uint32_t last_check_time;
    uint32_t max_time_in_checks;
    uint32_t os_id;
    uint32_t major_version;
    uint16_t uuid;
    uint16_t gid;
} super_block_t;

typedef struct {
    uint32_t first_inode;
    uint16_t inode_size;
    uint16_t block_group_idx;
    uint32_t features[3];
    char file_system_id[16];
    char volume_name[16];
    char last_mounted_path[64];
    uint32_t compress_algorithms;
    uint8_t preallocate_file_block_count;
    uint8_t preallocate_dir_block_count;
    uint8_t unused[818];
} super_block_ext_t;

typedef struct {
    uint32_t block_bitmap;
    uint32_t inode_bitmap;
    uint32_t inode_table;
    uint16_t unallocated_block_count;
    uint16_t unallocated_inode_count;
    uint16_t dir_count;
    uint8_t unused[14];
} block_group_desc_t;

#pragma pack()

#define EXT2_SIGNATURE 0xef53

#define STATE_NORMAL 0
#define STATE_CLEAN 1
#define STATE_ERRORS 2

#define ERROR_HANDING_IGNORE 1
#define ERROR_HANDING_REMOUNT 2
#define ERROR_HANDING_PANIC 3

#define OS_ID_LINUX 0
#define OS_ID_GNU_HURD 1
#define OS_ID_MASIC 2
#define OS_ID_FREE_BSD 3
#define OS_ID_OTHER 4

void log_ext2fs_info(super_block_t *super_block,
                     super_block_ext_t *super_block_ext) {
    if (super_block == NULL) {
        return;
    }
    log_info("Filessytem UUID:%d\n", super_block->uuid);
    log_info("Filesystem magic number:%d\n", super_block->ext2_sig);
    log_info("Filesystem state:%d\n", super_block->state);
    log_info("Errors behavior:%d\n", super_block->error_handling_method);
    log_info("Filesystem OS type:%d\n", super_block->os_id);
    log_info("Inode count:%d\n", super_block->inode_total);
    log_info("Block count:%d\n", super_block->blocks_total);
    log_info("Reserved Block count:%d\n", super_block->blocks_reserved_count);
    log_info("Free blocks:%d\n", super_block->unallocated_block_count);
    log_info("Free inodes:%d\n", super_block->unallocated_inode_count);
    log_info("First block:%d\n", super_block->super_block_id);
    log_info("Block size:%d\n", kb_size(super_block->block_size_kb));
    log_info("Fragment size:%d\n", kb_size(super_block->fragment_size_kb));
    log_info("Blocks per group:%d\n", super_block->blocks_per_group);
    log_info("Fragments per group:%d\n", super_block->fragments_per_group);
    log_info("Inodes per group:%d\n", super_block->inodes_per_group);
    log_info(
        "Inodes blocks per group:%d\n",
        super_block->inodes_per_group / (kb_size(super_block->block_size_kb) /
                                         super_block_ext->inode_size));
    log_info("Last mount time:%d\n", super_block->last_mount_time);
    log_info("Last write time:%d\n", super_block->last_written_time);
    log_info("Mount count:%d\n", super_block->mounts_since_last_check);
    log_info("Maximum mount count:%d\n",
             super_block->max_mounts_since_last_check);
    log_info("Last checked:%d\n", super_block->last_check_time);
    log_info("Check interal:%d\n", super_block->max_time_in_checks);
    log_info("First inode:%d\n", super_block_ext->first_inode);
    log_info("Inode size:%d\n\n", super_block_ext->inode_size);
}

void log_ext2_block_group_info(int i, super_block_t *super_block,
                               super_block_ext_t *super_block_ext,
                               block_group_desc_t *block_group) {
    size_t inodes_blocks_per_group = super_block->inodes_per_group *
                                     super_block_ext->inode_size /
                                     kb_size(super_block->block_size_kb);
    uint32_t block_start = i * super_block->blocks_per_group + 1;
    if (has_super_block(i)) {
        log_info("Grop %d:(block %d-%d)\n", i, block_start,
                 block_start + super_block->blocks_per_group - 1);
        log_info("Super block at %d, block group table at %d-%d\n", block_start,
                 block_start + 1, block_start + 1);
        log_info("Reverse GDT block at %d-%d\n", block_start + 2,
                 block_group->block_bitmap - 1);
    }
    log_info("Block bitmap at %d\n", block_group->block_bitmap);
    log_info("Inode bitmap at %d\n", block_group->inode_bitmap);
    log_info("Inode table at %d-%d\n", block_group->inode_table,
             block_group->inode_table + inodes_blocks_per_group - 1);
    log_info(
        "Unallocated block count %d, Unallocated inode count %d, dir count "
        "%d\n",
        block_group->unallocated_block_count,
        block_group->unallocated_inode_count, block_group->dir_count);
    log_info("Unallocated block: %d-%d\n",
             block_group->inode_table + inodes_blocks_per_group + 1,
             block_start + super_block->blocks_per_group - 1);
    size_t first_inode = i * super_block->inodes_per_group + 1;
    if (i == 0) {
        first_inode = super_block_ext->first_inode + 1;
    }
    log_info("Unallocated inode: %d-%d\n\n", first_inode,
             (i + 1) * super_block->inodes_per_group);
}

ssize_t block_read(super_block_t *super_block, dev_id_t dev_id, byte_t *buf,
                   off_t block_start, size_t block_count) {
    size_t block_size = (super_block->block_size_kb + 1) * KB(1);
    return device_read(dev_id, block_start * block_size / SECTOR_SIZE, buf,
                       block_count * block_size / SECTOR_SIZE);
}

error ext2_mount(fs_desc_t *fs, int major, int minor) {
    dev_id_t dev_id = device_open(major, minor, NULL);
    if (dev_id < 0) {
        goto mount_fail;
    }

    size_t size = sizeof(super_block_t) + sizeof(super_block_ext_t);
    super_block_t *super_block = (super_block_t *)vmalloc(size);
    if (!super_block) {
        goto mount_fail;
    }
    memset(super_block, 0, size);
    if (device_read(dev_id, KB(1) / SECTOR_SIZE, (byte_t *)super_block,
                    up2(size, SECTOR_SIZE) / SECTOR_SIZE) < 1) {
        goto mount_fail;
    }
    if (super_block->ext2_sig != EXT2_SIGNATURE) {
        goto mount_fail;
    }
    super_block_ext_t *super_block_ext =
        (super_block_ext_t *)((uint32_t)super_block + sizeof(super_block_t));
    log_ext2fs_info(super_block, super_block_ext);
    size_t block_group_count =
        super_block->inode_total / super_block->inodes_per_group;

    size = sizeof(block_group_desc_t);
    block_group_desc_t *block_groups = vmalloc(size * block_group_count);
    if (block_groups == NULL) {
        goto mount_fail;
    }
    size_t bgt_block_count =
        up2(block_group_count * size, kb_size(super_block->block_size_kb)) /
        kb_size(super_block->block_size_kb);
    if (block_read(super_block, dev_id, (byte_t *)block_groups, 2,
                   bgt_block_count) < 0) {
        goto mount_fail;
    }
    for (int i = 0; i < block_group_count; i++) {
        size_t cur_block = i * super_block->blocks_per_group + 1;
        block_group_desc_t *group = block_groups + i;

        log_ext2_block_group_info(i, super_block, super_block_ext, group);
    }
    fs->data = super_block;
    return 0;
mount_fail:
    if (super_block) {
        vfree(super_block);
    }
    if (block_groups) {
        vfree(block_groups);
    }

    return -1;
}

void ext2_unmount(fs_desc_t *fs) {
    if (!fs) {
        return;
    }
    if (fs->data) {
        vfree(fs->data);
    }
    if (fs->dev_id >= 0) {
        device_close(fs->dev_id);
    }
}

error ext2_open(fs_desc_t *fs, const char *path, file_t *file) {}
error ext2_ioctl(file_t *file, int cmd, int arg0, int arg1) {}
ssize_t ext2_read(file_t *file, byte_t *buf, size_t size) {}
ssize_t ext2_write(file_t *file, const byte_t *buf, size_t size) {}
void ext2_close(file_t *file) {}
off_t ext2_seek(file_t *file, off_t offset, int whence) {}
error ext2_stat(file_t *file, void *data) {}
error ext2_remove(file_t *file) {}
error ext2_readdir(file_t *dir, void *dirent) {}
error ext2_mkdir(fs_desc_t *fs, const char *path, file_t *file) {}
error ext2_link(file_t *file, const char *new_path, int arg) {}
error ext2_unlink(file_t *file_t) {}

fs_ops_t ext2_ops = {
    .mount = ext2_mount,
    .unmount = ext2_unmount,
    .open = ext2_open,
    .read = ext2_read,
    .write = ext2_write,
    .close = ext2_close,
    .seek = ext2_seek,
    .stat = ext2_stat,
    .readdir = ext2_readdir,
    .ioctl = ext2_ioctl,
    .remove = ext2_remove,
    .mkdir = ext2_mkdir,
    .link = ext2_link,
    .unlink = ext2_unlink,
};
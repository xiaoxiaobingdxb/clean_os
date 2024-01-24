#include "../device/disk/disk.h"
#include "../memory/malloc/mallocator.h"
#include "common/lib/bitmap.h"
#include "common/lib/string.h"
#include "common/tool/log.h"
#include "common/tool/math.h"
#include "common/types/byte.h"
#include "fs.h"
#include "include/syscall.h"

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
    uint16_t uid;
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

#define DIRECT_BLOCK_COUNT 12
#define INDIRECT_BLOCK_COUNT 3
#define ROOT_INODE 2
typedef struct {
    uint16_t type_permission;
    uint16_t uid;
    uint32_t size0_31;
    uint32_t last_access_time;
    uint32_t create_time;
    uint32_t last_write_time;
    uint32_t delete_time;
    uint16_t gid;
    uint16_t hard_link_count;
    uint32_t disk_sector_count;
    uint32_t flags;
    uint32_t os_spec1;
    uint32_t direct_blocks[DIRECT_BLOCK_COUNT];
    uint32_t indirect_blocks[INDIRECT_BLOCK_COUNT];
    uint32_t generation_num;
    uint32_t ext_attr_block;
    uint32_t size32_63;
    uint32_t block_addr_in_fragment;
    uint8_t os_spec2[12];
} inode_t;

typedef struct {
    super_block_t *super_block;
    super_block_ext_t *super_block_ext;
    block_group_desc_t *block_groups;
} ext2_desc;

typedef struct {
    inode_t *inode;
    size_t inode_idx;
} file_data_t;

typedef struct {
    uint32_t inode;
    uint16_t size;
    uint8_t name_length;
    uint8_t type;
} dir_entry_t;
typedef enum {
    Unknow,
    RegularFile,
    Directory,
    CharacterDevice,
    BlockDevice,
    Fifo,
    Socket,
    SoftLink
} dir_entry_type;

file_type get_file_type(dir_entry_type type) {
    switch (type) {
    case Unknow:
        return FILE_UNKNOWN;
    case RegularFile:
        return FILE_FILE;
    case Directory:
        return FILE_DIR;
    default:
        return FILE_UNKNOWN;
    }
}

#define dir_entry_name(entry) ((char *)((uint32_t)entry + sizeof(dir_entry_t)))
#define dir_entry_name_len(entry) (entry->size - 8)

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
    log_info("Filessytem UUID:%d\n", super_block->uid);
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

void log_ext2_block_group_info(int i, ext2_desc *desc) {
    super_block_t *super_block = desc->super_block;
    super_block_ext_t *super_block_ext = desc->super_block_ext;
    block_group_desc_t *block_groups = desc->block_groups;
    block_group_desc_t *block_group = block_groups + i;
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

ssize_t block_read(dev_id_t dev_id, size_t block_size, void *buf,
                   off_t block_start, size_t block_count) {
    return device_read(dev_id, block_start * block_size / SECTOR_SIZE, buf,
                       block_count * block_size / SECTOR_SIZE);
}

ssize_t block_write(dev_id_t dev_id, size_t block_size, void *buf,
                    off_t block_start, size_t block_count) {
    return device_write(dev_id, block_start * block_size / SECTOR_SIZE, buf,
                        block_count * block_size / SECTOR_SIZE);
}

error ext2_mount(fs_desc_t *fs, int major, int minor) {
    dev_id_t dev_id = device_open(major, minor, NULL);
    if (dev_id < 0) {
        goto mount_fail;
    }

    size_t size = sizeof(super_block_t) + sizeof(super_block_ext_t);

    super_block_t *super_block =
        (super_block_t *)kernel_mallocator.malloc(size);
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
    block_group_desc_t *block_groups = kernel_mallocator.malloc(
        up2(size * block_group_count, kb_size(super_block->block_size_kb)));
    if (block_groups == NULL) {
        goto mount_fail;
    }
    size_t bgt_block_count =
        up2(block_group_count * size, kb_size(super_block->block_size_kb)) /
        kb_size(super_block->block_size_kb);
    if (block_read(dev_id, kb_size(super_block->block_size_kb), block_groups, 2,
                   bgt_block_count) < 0) {
        goto mount_fail;
    }
    ext2_desc *desc = (ext2_desc *)kernel_mallocator.malloc(sizeof(ext2_desc));
    if (!desc) {
        goto mount_fail;
    }
    desc->super_block = super_block;
    desc->super_block_ext = super_block_ext;
    desc->block_groups = block_groups;
    for (int i = 0; i < block_group_count; i++) {
        log_ext2_block_group_info(i, desc);
    }
    fs->data = desc;
    fs->type = FS_EXT2;
    fs->dev_id = dev_id;
    return 0;
mount_fail:
    if (super_block) {
        kernel_mallocator.free(super_block);
    }
    if (block_groups) {
        kernel_mallocator.free(block_groups);
    }
    if (desc) {
        kernel_mallocator.free(desc);
    }
    return -1;
}

void ext2_unmount(fs_desc_t *fs) {
    if (!fs) {
        return;
    }
    if (fs->data) {
        ext2_desc *desc = (ext2_desc *)fs->data;
        kernel_mallocator.free(desc->super_block);
        kernel_mallocator.free(desc->block_groups);
        kernel_mallocator.free(desc);
    }
    if (fs->dev_id >= 0) {
        device_close(fs->dev_id);
    }
}

error rw_inode(dev_id_t dev_id, ext2_desc *desc, uint32_t inode_idx,
               inode_t *inode, bool write) {
    inode_idx--;
    off_t group_idx = inode_idx / desc->super_block->inodes_per_group;
    inode_idx = inode_idx % desc->super_block->inodes_per_group;

    size_t block_size = kb_size(desc->super_block->block_size_kb);

    size_t inode_block_idx =
        inode_idx * desc->super_block_ext->inode_size / block_size;
    inode_block_idx += desc->block_groups[group_idx].inode_table;
    size_t buf_size = max(desc->super_block_ext->inode_size, block_size);
    byte_t *inode_buf = (byte_t *)kernel_mallocator.malloc(buf_size);
    if (!inode_buf) {
        return -1;
    }
    uint32_t inode_offset =
        inode_idx * desc->super_block_ext->inode_size % block_size;
    error err = 0;
    ssize_t read_size =
        block_read(dev_id, block_size, inode_buf, inode_block_idx, 1);
    if (read_size <= 0) {
        err = -1;
        goto finally;
    }
    if (write) {
        memcpy(inode_buf + inode_offset, inode, min(buf_size, sizeof(inode_t)));
        ssize_t write_size =
            block_write(dev_id, block_size, inode_buf, inode_block_idx, 1);
        if (write_size <= 0) {
            err = -1;
            goto finally;
        }
    } else {
        memcpy(inode, inode_buf + inode_offset, min(buf_size, sizeof(inode_t)));
    }
finally:
    if (inode_buf) {
        kernel_mallocator.free(inode_buf);
    }
    return err;
}

ssize_t get_sub_inode(fs_desc_t *fs, ext2_desc *desc, inode_t *parent,
                      char *name, inode_t *inode) {
    dir_entry_t *entries =
        kernel_mallocator.malloc(kb_size(desc->super_block->block_size_kb));
    if (!entries) {
        return -1;
    }
    ssize_t idx = -1;
    for (int i = 0; i < DIRECT_BLOCK_COUNT && parent->direct_blocks[i] > 0;
         i++) {
        if (block_read(fs->dev_id, kb_size(desc->super_block->block_size_kb),
                       entries, parent->direct_blocks[i], 1) <= 0) {
            idx = -1;
            goto finally;
        }
        for (dir_entry_t *entry = entries;
             entry->inode > 0 && (uint32_t)entry - (uint32_t)entries <
                                     kb_size(desc->super_block->block_size_kb);
             entry = (dir_entry_t *)((uint32_t)entry + entry->size)) {
            char *get_name = dir_entry_name(entry);
            if (memcmp(name, get_name,
                       min(strlen(name), dir_entry_name_len(entry))) == 0) {
                idx = rw_inode(fs->dev_id, desc, entry->inode, inode, false);
                if (idx == 0) {
                    idx = entry->inode;
                }
                goto finally;
            }
        }
    }
finally:
    kernel_mallocator.free(entries);
    return idx;
}

error extract_file(file_t *file, inode_t *inode) {
    if (inode->type_permission & 0x4000) {
        file->type = FILE_DIR;
    } else if (inode->type_permission & 0x8000) {
        file->type = FILE_FILE;
    } else {
        file->type = FILE_UNKNOWN;
    }
    file->size = inode->size0_31;
    file->pos = 0;
    return 0;
}

error ext2_open(fs_desc_t *fs, const char *path, file_t *file) {
    ext2_desc *desc = (ext2_desc *)fs->data;
    if (!desc) {
        return -1;
    }
    file_data_t *file_data = NULL;
    error err;
    inode_t *root_inode =
        kernel_mallocator.malloc(desc->super_block_ext->inode_size);
    if (!root_inode) {
        err = -1;
        goto open_fail;
    }
    if ((err = rw_inode(fs->dev_id, desc, ROOT_INODE, root_inode, false)) !=
        0) {
        goto open_fail;
    }
    inode_t *inode = root_inode;
    char *name = NULL;
    ssize_t inode_idx = ROOT_INODE;
    if (strlen(path) == 0 || !strcmp(path, "/") || !strcmp(path, "/.") ||
        !strcmp(path, "/..")) {  // root
        memcpy(file->name, "/", 1);
    } else {
        char *save_ptr;
        char *str = strtok_r((char *)path, "/", &save_ptr);
        while (str != NULL) {
            memcpy(file->name, str, strlen(str));
            inode_t tmp;
            if ((inode_idx = get_sub_inode(fs, desc, inode, str, &tmp)) < 0) {
                err = inode_idx;
                goto open_fail;
            }
            memcpy(inode, &tmp, sizeof(inode_t));
            str = strtok_r(NULL, "/", &save_ptr);
        }
    }
    extract_file(file, inode);
    file_data = (file_data_t *)kernel_mallocator.malloc(sizeof(file_data_t));
    if (!file_data) {
        err = -1;
        goto open_fail;
    }
    file_data->inode = inode;
    file_data->inode_idx = inode_idx;
    file->data = file_data;
    if (file->mode & O_APPEND) {
        file->pos = file->size;
    }
    return 0;
open_fail:
    if (root_inode) {
        kernel_mallocator.free(root_inode);
    }
    if (file_data) {
        kernel_mallocator.free(file_data);
    }
    return err;
}
ssize_t ext2_read(file_t *file, byte_t *buf, size_t size) {
    if (file->type != FILE_FILE || file->data == NULL || file->desc == NULL) {
        return -1;
    }
    fs_desc_t *fs = file->desc;
    if (fs->data == NULL) {
        return -1;
    }
    ext2_desc *desc = (ext2_desc *)fs->data;
    size_t block_size = kb_size(desc->super_block->block_size_kb);
    file_data_t *file_data = (file_data_t *)file->data;
    inode_t *inode = file_data->inode;
    byte_t *read_buf = kernel_mallocator.malloc(block_size);
    if (!read_buf) {
        return -1;
    }
    size = min(size, file->size - file->pos);
    if (size <= 0) {
        return 0;
    }
    ssize_t total_size = 0;
    for (int i = file->pos / block_size; size > 0 && i < DIRECT_BLOCK_COUNT;
         i++) {
        if (inode->direct_blocks[i] == 0) {
            continue;
        }
        off_t offset = file->pos % block_size;
        ssize_t read_size = block_read(fs->dev_id, block_size, read_buf,
                                       inode->direct_blocks[i], 1);
        if (read_size <= 0) {
            total_size = read_size;
            goto finally;
        }
        read_size = min(size, block_size - offset);
        memcpy(buf, read_buf + offset, read_size);
        file->pos += read_size;
        size -= read_size;
        buf += read_size;
        total_size += read_size;
    }
finally:
    kernel_mallocator.free(read_buf);
    return total_size;
}

size_t alloc_block(dev_id_t dev_id, ext2_desc *desc, size_t count,
                   uint32_t *blocks, bool must) {
    block_group_desc_t *group = desc->block_groups + 0;
    if (group->unallocated_block_count < count) {
        return 0;
    }
    size_t block_size = kb_size(desc->super_block->block_size_kb);
    byte_t *block_bitmap = kernel_mallocator.malloc(block_size);
    if (!block_bitmap) {
        return 0;
    }
    block_read(dev_id, block_size, block_bitmap, group->block_bitmap, 1);
    bitmap_t bitmap;
    bitmap.bits = block_bitmap;
    bitmap.bytes_len = block_size;
    size_t success = 0;
    size_t inodes_blocks_per_group = desc->super_block->inodes_per_group *
                                     desc->super_block_ext->inode_size /
                                     block_size;
    size_t data_block_start = group->inode_table + inodes_blocks_per_group + 1;
    for (int i = 0; i < count; i++) {  // search one by one
        int idx = bitmap_scan(&bitmap, 1);
        if (idx >= 0) {
            bitmap_set(&bitmap, idx, 1);
            success++;
            blocks[i] = idx + data_block_start;
        } else if (must) {
            return 0;
        }
    }
    group->unallocated_block_count -= success;
    desc->super_block->unallocated_block_count -= success;
    // block_write(dev_id, block_size, block_bitmap, group->block_bitmap, 1);
    // block_write(dev_id, block_size, desc->super_block, 2, 1);
    // block_write(dev_id, block_size, desc->block_groups, 1, 1);
    kernel_mallocator.free(block_bitmap);
    return success;
}

ssize_t ext2_write(file_t *file, const byte_t *buf, size_t size) {
    if (file->mode & O_RDONLY) {
        return -1;
    }
    if (!file->data || !file->desc->data) {
        return -1;
    }
    file_data_t *file_data = (file_data_t *)file->data;
    inode_t *inode = (inode_t *)file_data->inode;
    ext2_desc *desc = (ext2_desc *)file->desc->data;

    uint32_t block_size = kb_size(desc->super_block->block_size_kb);
    sint32_t alloc_size =
        (sint32_t)file->pos + size - up2(file->size, block_size);
    if (alloc_size > 0) {  // file free space is not enough
        // to alloc more free space
        alloc_size = up2(alloc_size, block_size);
        size_t alloc_block_count = alloc_size / block_size;
        uint32_t *blocks =
            kernel_mallocator.malloc(sizeof(uint32_t) * alloc_block_count);
        if (alloc_block(file->desc->dev_id, desc, alloc_block_count, blocks,
                        true) == alloc_block_count) {
            int block_start = up2(file->size, block_size) / block_size + 1;
            for (int i = 0; i < alloc_block_count; i++) {
                inode->direct_blocks[block_start + i] = blocks[i];
            }
        }
        kernel_mallocator.free(blocks);
        // block_write(file->desc->dev_id, block_size, inode, 0, 1);
    }
    size_t block_idx = file->pos / block_size;
    off_t block_offset = file->pos % block_size;
    byte_t *buffer = (byte_t *)kernel_mallocator.malloc(block_size);
    size_t write_size = 0;
    size_t all_write_size = 0;
    if (block_offset) {
        write_size = min(block_size - block_offset, size);
        block_read(file->desc->dev_id, block_size, buffer,
                   inode->direct_blocks[block_idx], 1);
        memcpy(buffer + block_offset, buf, write_size);
        block_write(file->desc->dev_id, block_size, buffer,
                    inode->direct_blocks[block_idx], 1);
        size -= write_size;
        file->pos += write_size;
        file->size = max(file->size, file->pos);
        inode->size0_31 = file->size;
        buf += write_size;
        all_write_size += write_size;
    }
    while (size > 0) {
        block_idx = file->pos / block_size;
        block_offset = file->pos % block_size;

        write_size = min(block_size - block_offset, size);
        block_read(file->desc->dev_id, block_size, buffer,
                   inode->direct_blocks[block_idx], 1);
        memcpy(buffer + block_offset, buf, write_size);
        block_write(file->desc->dev_id, block_size, buffer,
                    inode->direct_blocks[block_idx], 1);
        size -= write_size;
        file->pos += write_size;
        file->size = max(file->size, file->pos);
        inode->size0_31 = file->size;
        buf += write_size;
        all_write_size += write_size;
    }
    error err =
        rw_inode(file->desc->dev_id, desc, file_data->inode_idx, inode, true);
    if (err) {
        return err;
    }
    return all_write_size;
}
void ext2_close(file_t *file) {}
off_t ext2_seek(file_t *file, off_t offset, int whence) {
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
error ext2_stat(file_t *file, void *data) {
    if (file->data == NULL || file->desc == NULL) {
        return -1;
    }
    file_data_t *file_data = (file_data_t *)file->data;
    inode_t *inode = (inode_t *)file_data->inode;
    ext2_desc *desc = (ext2_desc *)file->desc->data;
    stat_t *stat = (stat_t *)data;
    stat->dev_id = file->desc->dev_id;
    stat->size = file->size;
    stat->block_size = kb_size(desc->super_block->block_size_kb);
    stat->block_count = 0;
    for (int i = 0; i < DIRECT_BLOCK_COUNT; i++) {
        if (inode->direct_blocks[i] > 0) {
            stat->block_count++;
        }
    }
    stat->create_time.tv_sec = (time64_t)inode->create_time;
    stat->create_time.tv_nsec = 0;
    stat->update_time.tv_sec = (time64_t)inode->last_write_time;
    stat->update_time.tv_nsec = 0;
    return 0;
}
error ext2_remove(file_t *file) {}
error ext2_readdir(file_t *dir, void *data) {
    if (dir->type != FILE_DIR || dir->data == NULL || dir->desc == NULL ||
        data == NULL) {
        return -1;
    }
    dirent_t *dirent = (dirent_t *)data;
    file_data_t *file_data = (file_data_t *)dir->data;
    inode_t *inode = (inode_t *)file_data->inode;
    fs_desc_t *fs = dir->desc;
    if (fs->data == NULL) {
        return -1;
    }
    ext2_desc *desc = (ext2_desc *)fs->data;

    dir_entry_t *entries =
        kernel_mallocator.malloc(kb_size(desc->super_block->block_size_kb));
    dirent->offset;
    int i = 0;
    bool found = false;
    for (; i < DIRECT_BLOCK_COUNT && inode->direct_blocks[i] &&
           inode->direct_blocks[i] > 0;
         i++) {
        if (block_read(fs->dev_id, kb_size(desc->super_block->block_size_kb),
                       entries, inode->direct_blocks[i], 1) <= 0) {
            return -1;
        }
        uint32_t off = 0;
        int file_idx = 0;
        dir_entry_t *entry = entries;
        for (; file_idx < dirent->offset &&
               off + entry->size < kb_size(desc->super_block->block_size_kb);
             file_idx++) {
            off += entry->size;
            entry = (dir_entry_t *)((uint32_t)entry + entry->size);
        }
        if (file_idx == dirent->offset) {  // have found
            found = true;
            dirent->offset++;
            dirent->type = get_file_type((dir_entry_type)entry->type);
            char *name = dir_entry_name(entry);
            memcpy(dirent->name, name,
                   min(sizeof(dirent->name), dir_entry_name_len(entry)));
            inode_t *child_inode = kernel_mallocator.malloc(sizeof(inode_t));
            rw_inode(fs->dev_id, desc, entry->inode, child_inode, false);
            dirent->size = child_inode->size0_31;
            kernel_mallocator.free(child_inode);
            goto success;
        }
    }
success:
    kernel_mallocator.free(entries);
    if (found) {
        return 0;
    }
    return -1;
}
error ext2_mkdir(fs_desc_t *fs, const char *path, file_t *file) {}
error ext2_link(file_t *file, const char *new_path, int arg) {}
error ext2_unlink(file_t *file_t) {}
error ext2_ioctl(file_t *file, int cmd, int arg0, int arg1) {}
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
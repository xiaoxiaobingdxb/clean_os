#include "./fs.h"

#include "../device/device.h"
#include "../device/disk/disk.h"
#include "../syscall/syscall_user.h"
#include "../thread/thread.h"
#include "common/lib/string.h"
#include "common/tool/lib.h"

#define FS_TABLE_SIZE 10
fs_desc_t fs_table[FS_TABLE_SIZE];
fs_ops_t *fs_ops_table[FS_TABLE_SIZE];
list free_fs, mounted_fs;
void init_fs_table() {
    init_list(&free_fs);
    for (int i = 0; i < FS_TABLE_SIZE; i++) {
        pushr(&free_fs, &fs_table[i].node);
    }
    init_list(&mounted_fs);
    extern fs_ops_t devfs_ops, fat16_ops, ext2_ops;
    memset(&fs_ops_table, 0, sizeof(fs_ops_table));
    fs_ops_table[FS_DEV] = &devfs_ops;
    fs_ops_table[FS_FAT16] = &fat16_ops;
    fs_ops_table[FS_EXT2] = &ext2_ops;
}
void init_fs() {
    init_fs_table();
    init_disk();

    mount(FS_DEV, "/dev", DEV_TTY, 0);  // mount default /dev
    mount(FS_FAT16, "/home", DEV_DISK, 0x11);
    mount(FS_EXT2, "/ext2", DEV_DISK, 0x20);
}

bool mounted_fs_visitor_by_path(list_node *node, void *arg) {
    void **args = (void **)arg;
    const char *path = (const char *)args[0];
    fs_desc_t *fs = tag2entry(fs_desc_t, node, node);
    if (!memcmp(fs->mount_point, path, strlen(fs->mount_point))) {
        args[1] = fs;
        return false;
    }
    return true;
}

fs_desc_t *mount(fs_type_t type, const char *mount_point, int dev_major,
                 int dev_minor) {
    void *args[2] = {(void *)mount_point, NULL};
    foreach (&mounted_fs, mounted_fs_visitor_by_path, (void *)args)
        ;
    fs_desc_t *fs;
    if (!args[1]) {  // not found mounted fs, to create a new fs
        list_node *fs_node = popl(&free_fs);
        if (!fs_node) {
            goto mount_fail;
        }
        fs = tag2entry(fs_desc_t, node, fs_node);
        memcpy(fs->mount_point, mount_point, FS_MOUNTP_SIZE);
        fs_ops_t *ops = fs_ops_table[type];
        if (!ops) {
            goto mount_fail;
        }
        fs->ops = ops;
        if (fs->ops->mount(fs, dev_major, dev_minor)) {
            goto mount_fail;
        }
        pushr(&mounted_fs, &fs->node);
    } else {
        // had been mounted, fail
        goto mount_fail;
    }
    return fs;

mount_fail:
    if (fs) {
        pushr(&free_fs, &fs->node);
    }
    return NULL;
}

fd_t sys_open(const char *name, int flag, ...) {
    void *args[2] = {(void *)name, NULL};
    foreach (&mounted_fs, mounted_fs_visitor_by_path, (void *)args)
        ;
    if (!args[1]) {
        goto open_fail;
    }
    fs_desc_t *fs = (fs_desc_t *)args[1];
    file_t *file = alloc_file();
    if (!file) {
        goto open_fail;
    }
    fd_t fd = task_alloc_fd(file);
    if (fd < 0) {
        goto open_fail;
    }
    file->mode = flag;
    file->desc = fs;
    name = path_next_child(name);
    memcpy(file->name, name, 0);
    if (fs->ops->open(fs, name, file)) {
        goto open_fail;
    }
    return fd;
open_fail:
    if (file) {
        free_file(file);
    }
    return FILENO_UNKNOWN;
}

fd_t sys_dup(fd_t fd) {
    file_t *file = task_file(fd);
    if (!file) {
        return EBADF;
    }
    fd_t new_fd = task_alloc_fd(file);
    if (new_fd >= 0) {
        ref_file(file);
        return new_fd;
    }
    return -1;
}
fd_t sys_dup2(fd_t dst, fd_t source) {
    if (dst == source) {
        return dst;
    }
    file_t *file = task_file(source);
    if (!file) {
        return EBADF;
    }
    file_t *old = task_file(dst);
    if (old) {
        free_file(file);
        task_free_fd(dst);
    }
    if (task_set_file(dst, file, true) == 0) {
        ref_file(file);
        return dst;
    }
}

ssize_t sys_write(fd_t fd, const void *buf, size_t size) {
    file_t *file = task_file(fd);
    if (!file) {
        return EBADF;
    }
    return file->desc->ops->write(file, buf, size);
}

ssize_t sys_read(fd_t fd, void *buf, size_t size) {
    file_t *file = task_file(fd);
    if (!file) {
        return EBADF;
    }
    return file->desc->ops->read(file, buf, size);
}

off_t sys_lseek(fd_t fd, off_t offset, int whence) {
    file_t *file = task_file(fd);
    if (!file) {
        return EBADF;
    }
    return file->desc->ops->seek(file, offset, whence);
}

#include "../net/socket.h"
error sys_close(fd_t fd) {
    socket_t* socket = get_socket(fd);
    if (socket) {
        return close_socket(fd);
    }
    file_t *file = task_file(fd);
    if (!file) {
        return EBADF;
    }
    ASSERT(file->ref > 0);
    free_file(file);
    if (file->ref == 0) {
        file->desc->ops->close(file);
    }
    task_free_fd(fd);
    return 0;
}

error sys_stat(const char *name, void *data) {
    fd_t fd = sys_open(name, O_RDONLY);
    int ret = sys_fstat(fd, data);
    close(fd);
    return ret;
}

error sys_fstat(fd_t fd, void *data) {
    file_t *file = task_file(fd);
    if (!file) {
        return EBADF;
    }
    return file->desc->ops->stat(file, data);
}

error sys_readdir(fd_t fd, void *dirent) {
    file_t *file = task_file(fd);
    if (!file) {
        return EBADF;
    }
    if (!dirent) {
        return -1;
    }
    return file->desc->ops->readdir(file, dirent);
}

error sys_mkdir(const char *path) {
    void *args[2] = {(void *)path, NULL};
    foreach (&mounted_fs, mounted_fs_visitor_by_path, (void *)args)
        ;
    if (!args[1]) {
        goto open_fail;
    }
    fs_desc_t *fs = (fs_desc_t *)args[1];
    file_t *file = alloc_file();
    if (!file) {
        goto open_fail;
    }
    fd_t fd = task_alloc_fd(file);
    if (fd < 0) {
        goto open_fail;
    }
    file->desc = fs;
    path = path_next_child(path);
    memcpy(file->name, path, 0);
    if (fs->ops->mkdir(fs, path, file)) {
        goto open_fail;
    }
    return fd;
open_fail:
    if (!file) {
        free_file(file);
    }
    return FILENO_UNKNOWN;
}

error sys_rmdir(const char *path) {
    fd_t fd = open(path, 0);
    if (fd < 0) {
        return fd;
    }
    file_t *file = task_file(fd);
    if (!file) {
        return EBADF;
    }
    return file->desc->ops->remove(file);
}

error _sys_link(const char *new_path, const char *old_path, int link_type) {
    fd_t fd = open(old_path, 0);
    if (fd < 0) {
        return fd;
    }
    file_t *file = task_file(fd);
    if (!file) {
        return EBADF;
    }
    return file->desc->ops->link(file, new_path, link_type);
}

error sys_link(const char *new_path, const char *old_path) {
    return _sys_link(new_path, old_path, 0);
}

error sys_symlink(const char *new_path, const char *old_path) {
    return _sys_link(new_path, old_path, 1);
}

error sys_unlink(const char *path) {
    fd_t fd = open(path, 0);
    if (fd < 0) {
        return fd;
    }
    file_t *file = task_file(fd);
    if (!file) {
        return EBADF;
    }
    return file->desc->ops->unlink(file);
}
#if !defined(FS_FS_H)
#define FS_FS_H

#include "../device/device.h"
#include "common/types/basic.h"
#include "file.h"
#include "common/lib/list.h"

#define FS_MOUNTP_SIZE 512
typedef enum {
    FS_DEV,
} fs_type_t;
struct _fs_ops_t;
typedef struct _fs_desc_t {
    char mount_point[FS_MOUNTP_SIZE];
    fs_type_t type;
    list_node node;  // organize by link_list
    struct _fs_ops_t *ops;
} fs_desc_t;
typedef struct _fs_ops_t {
    int (*mount)(fs_desc_t *fs, int major, int minor);
    void (*unmount)(fs_desc_t *fs);
    int (*open)(fs_desc_t *fs, const char *path, file_t *file);
    ssize_t (*read)(file_t *file, byte_t *buf, size_t size);
    ssize_t (*write)(file_t *file, const byte_t *buf, size_t size);
    void (*close)(file_t *file);
    int (*seek)(file_t *file, size_t offset);
    int (*ioctl)(file_t *file, int cmd, int arg0, int arg1);
} fs_ops_t;

void init_fs();

fs_desc_t *mount(fs_type_t type, const char *mount_point, int dev_major,
                 int dev_minor);

fd_t sys_open(const char *name, int flag, ...);

ssize_t sys_write(fd_t fd, const void *buf, size_t size);

ssize_t sys_read(fd_t fd, void *buf, size_t size);

int sys_close(fd_t fd);

fd_t sys_dup(fd_t fd);
fd_t sys_dup2(fd_t dst, fd_t source);

#endif  // FS_FS_H

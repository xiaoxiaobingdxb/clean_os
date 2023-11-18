#include "../device/device.h"
#include "fs.h"
#include "common/lib/string.h"

typedef struct {
    char *name;
    file_type file_type;
    device_type device_type;
} devfs_type_t;

devfs_type_t devfs_type_table[] = {{
    .name = "tty",
    .file_type = FILE_TTY,
    .device_type = DEV_TTY,
}};

int devfs_mount(fs_desc_t *fs, int major, int minor) {
    fs->type = FS_DEV;
    return 0;
}
void devfs_unmount(fs_desc_t *fs) {}
int devfs_open(fs_desc_t *fs, const char *path, file_t *file) {
    for (int i = 0; i < sizeof(devfs_type_table) / sizeof(devfs_type_t); i++) {
        devfs_type_t type = devfs_type_table[i];
        size_t len = strlen(type.name);
        if (memcmp(type.name, path, len)) {
            continue;
        }
        int minor = 0;
        if (strlen(path) <= len || str2num(path + len, &minor)) {
            minor = 0;
        }
        dev_id_t dev_id = device_open(type.device_type, minor, &file->mode);
        if (dev_id < 0) {
            return -1;
        }
        file->dev_id = dev_id;
        file->pos = 0;
        file->type = type.file_type;
        return 0;
    }
    return -1;
}
ssize_t devfs_read(file_t *file, byte_t *buf, size_t size) {
    return device_read(file->dev_id, file->pos, buf, size);
}
ssize_t devfs_write(file_t *file, const byte_t *buf, size_t size) {
    return device_write(file->dev_id, file->pos, buf, size);
}
void devfs_close(file_t *file) {
    device_close(file->dev_id);
}
int devfs_seek(file_t *file, size_t offset) {
    return -1;
}
int devfs_ioctl(file_t *file, int cmd, int arg0, int arg1) {
    return device_control(file->dev_id, cmd, arg0, arg1);
}

fs_ops_t devfs_ops = {
    .mount = devfs_mount,
    .unmount = devfs_unmount,
    .open = devfs_open,
    .read = devfs_read,
    .write = devfs_write,
    .close = devfs_close,
    .seek = devfs_seek,
    .ioctl = devfs_ioctl,
};
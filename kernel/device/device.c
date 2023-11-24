#include "device.h"

#include "common/lib/string.h"
#include "common/types/std.h"

extern device_desc_t dev_tty_desc;
extern device_desc_t dev_disk_desc;
device_desc_t *device_desc_table[] = {&dev_tty_desc, &dev_disk_desc};
#define DEVICES_SIZE 128
device_t devices[DEVICES_SIZE];

dev_id_t device_open(int major, int minor, void *arg) {
    device_t *free_dev = NULL;
    for (int i = 0; i < DEVICES_SIZE; i++) {
        device_t *dev = devices + i;
        if (dev->open_count == 0) {
            free_dev = dev;
        } else if (dev->desc->major == major && dev->minor == minor) {
            dev->open_count++;
            return i;
        }
    }
    device_desc_t *desc = NULL;
    int desc_size = sizeof(device_desc_table) / sizeof(device_desc_t*);
    for (int i = 0; i < desc_size; i++) {
        if (device_desc_table[i]->major == major) {
            desc = device_desc_table[i];
            break;
        }
    }
    if (desc && free_dev) {
        free_dev->desc = desc;
        free_dev->minor = minor;
        free_dev->arg = arg;
        if (free_dev->desc->open(free_dev) == 0) {
            free_dev->open_count = 1;
            return free_dev - devices;
        }
    }
    return -1;
}

bool validate_device(dev_id_t dev_id) {
    if (dev_id < 0 || dev_id >= sizeof(devices) / sizeof(device_t)) {
        return false;
    }
    if ((devices + dev_id)->desc == NULL) {
        return false;
    }
    return true;
}
ssize_t device_write(dev_id_t dev_id, uint32_t addr, const byte_t *buf, size_t size) {
    if (!validate_device(dev_id)) {
        return -1;
    }
    return (devices + dev_id)->desc->write((devices + dev_id), addr, buf, size);
}
ssize_t device_read(dev_id_t dev_id, uint32_t addr, byte_t *buf, size_t size) {
    if (!validate_device(dev_id)) {
        return -1;
    }
    return (devices + dev_id)->desc->read((devices + dev_id), addr, buf, size);
}
int device_control(dev_id_t dev_id, int cmd, int arg0, int arg1) {
    if (!validate_device(dev_id)) {
        return -1;
    }
    return (devices + dev_id)->desc->control((devices + dev_id), cmd, arg0, arg1);
}
void device_close(dev_id_t dev_id) {
    if (!validate_device(dev_id)) {
        return;
    }
    device_t *dev = devices + dev_id;
    if (--dev->open_count == 0) {
        dev->desc->close(dev);
        memset(dev, 0, sizeof(device_t));
    }
}
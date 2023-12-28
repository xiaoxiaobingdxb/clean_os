#ifndef DEVICE_DEIVCE_H
#define DEVICE_DEIVCE_H

#include "common/types/basic.h"
#include "include/fs_model.h"

typedef struct _device_t {
    struct _device_desc_t *desc;
    int minor;
    void *arg;
    int open_count;
    void *data;
} device_t;
#define DEVICE_NAME_SIZE 32
typedef struct _device_desc_t {
    char name[DEVICE_NAME_SIZE];
    int major;
    error (*open) (struct _device_t*);
    ssize_t (*write) (struct _device_t*, uint32_t, const byte_t*, size_t);
    ssize_t (*read) (struct _device_t*, uint32_t, byte_t*, size_t);
    error (*control) (struct _device_t*, int, int, int);
    void (*close) (struct _device_t*);
} device_desc_t;

typedef enum {
    DEV_UNKNOWN = 0,
    DEV_TTY,
    DEV_DISK,
} device_type;

dev_id_t device_open(int major, int minor, void *arg);
ssize_t device_write(dev_id_t dev_id, uint32_t addr, const byte_t *buf, size_t size);
ssize_t device_read(dev_id_t dev_id, uint32_t addr, byte_t *buf, size_t size);
int device_control(dev_id_t dev_id, int cmd, int arg0, int arg1);
void device_close(dev_id_t dev_id);
#endif
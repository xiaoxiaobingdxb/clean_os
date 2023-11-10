#ifndef DEVICE_DEIVCE_H
#define DEVICE_DEIVCE_H

#include "common/types/basic.h"

typedef struct _device_t {
    struct _device_desc_t *desc;
    int minor;
    void *arg;
    int open_count;
} device_t;
#define DEVICE_NAME_SIZE 32
typedef struct _device_desc_t {
    char name[DEVICE_NAME_SIZE];
    int major;
    int (*open) (struct _device_t*);
    ssize_t (*write) (struct _device_t*, uint32_t, byte_t*, size_t);
    ssize_t (*read) (struct _device_t*, uint32_t, byte_t*, size_t);
    int (*control) (struct _device_t*, int, int, int);
    void (*close) (struct _device_t*);
} device_desc_t;

enum {
    DEV_UNKNOWN = 0,
    DEV_TTY
};

int device_open(int major, int minor, void *arg);
ssize_t device_write(int dev_id, uint32_t addr, byte_t *buf, size_t size);
ssize_t device_read(int dev_id, uint32_t addr, byte_t *buf, size_t size);
int device_control(int dev_id, int cmd, int arg0, int arg1);
void device_close(int dev_id);
#endif
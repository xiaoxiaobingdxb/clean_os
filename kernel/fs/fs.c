#include "./fs.h"

#include "../device/device.h"
#include "../syscall/syscall_user.h"
#include "common/tool/lib.h"

int stdio;
void init_stdio() {
    stdio = device_open(DEV_TTY, 0, NULL);
    ASSERT(stdio >= 0);
}
void init_fs() {
    init_stdio();
}


ssize_t sys_write(int fd, const void *buf, size_t size) {
    if (fd == STDOUT_FILENO || fd == STDERR_FILENO) {
        return device_write(stdio, 0, (byte_t*)buf, size);
    }
    return -1;
}

ssize_t sys_read(int fd, const void *buf, size_t size) {
    if (fd == STDIN_FILENO) {
        return device_read(stdio, 0, (byte_t*)buf, size);
    }
    return -1;
}
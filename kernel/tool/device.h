#if !defined(TOOL_DEVICE_H)
#define TOOL_DEVICE_H
#include "include/syscall.h"

static inline void init_tty() {
    fd_t fd = open("/dev/tty0", 0);
    dup2(STDIN_FILENO, fd);
    dup2(STDOUT_FILENO, fd);
    dup2(STDERR_FILENO, fd);
}

#endif // TOOL_DEVICE_H
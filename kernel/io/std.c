#include "../fs/fs.h"
#include "../syscall/syscall_user.h"
#include "common/lib/stdio.h"
#include "../memory/malloc/malloc.h"

void init_std() {
    fd_t fd = open("/dev/tty0", 0);
    dup2(STDIN_FILENO, fd);
    dup2(STDOUT_FILENO, fd);
    dup2(STDERR_FILENO, fd);
}

ssize_t printf(const char *fmt, ...) {
    size_t size = sizeof(fmt) + 128;
    char *buf = (char*)malloc(size);
    va_list args;
    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);
    return write(STDOUT_FILENO, buf, size);
}

ssize_t scanf(const char *fmt, ...) {

}
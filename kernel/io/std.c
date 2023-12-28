#include "../fs/fs.h"
#include "../memory/malloc/malloc.h"
#include "../syscall/syscall_user.h"
#include "common/lib/stdio.h"
#include "common/lib/string.h"

void init_std() {
    fd_t fd = open("/dev/tty0", 0);
    dup2(STDIN_FILENO, fd);
    dup2(STDOUT_FILENO, fd);
    dup2(STDERR_FILENO, fd);
}

ssize_t _fprintf(const fd_t fd, const char *fmt, va_list args) {
    size_t size = sizeof(fmt) + 128;
    char *buf = (char *)malloc(size);
    vsprintf(buf, fmt, args);
    ssize_t write_size =  write(fd, buf, size);
    free(buf);
    return write_size;
}
ssize_t fprintf(const fd_t fd, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    ssize_t write_size = _fprintf(fd, fmt, args);
    va_end(args);
    return write_size;
}
ssize_t printf(const char *fmt, ...) {
    size_t size = sizeof(fmt) + 128;
    char *buf = (char *)malloc(size);
    va_list args;
    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);
    ssize_t write_size =  write(STDOUT_FILENO, buf, strlen(buf));
    free(buf);
    return write_size;
}

char getchar() {
    char buf[1];
    ssize_t read_size = read(STDIN_FILENO, buf, 1);
    return buf[0];
}

char *gets(char *buf) {
    char ch;
    size_t idx = 0;
    while (true) {
        ssize_t read_size = read(STDIN_FILENO, &ch, 1);
        if (read_size <= 0) {
            return NULL;
        }
        buf[idx++] = ch;
        if (ch == '\n'||ch == '\r') {
            buf[idx - 1] = 0;
            break;
        }
    }
    return buf;
}

ssize_t scanf(const char *fmt, ...) {}
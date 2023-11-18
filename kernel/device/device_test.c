#include "../syscall/syscall_user.h"
#include "common/lib/stdio.h"
#include "common/lib/string.h"
#include "common/tool/log.h"

void test_tty() {
    fd_t fd = open("/dev/tty0", 0);
    dup2(STDIN_FILENO, fd);
    dup2(STDOUT_FILENO, fd);
    dup2(STDERR_FILENO, fd);
    char *str = "test_stdout\n";
    ssize_t count = write(STDOUT_FILENO, str, strlen(str));
    str = "test_stderr\n";
    count = write(STDERR_FILENO, str, strlen(str));
    str = mmap_anonymous(10);
    memset(str, 0, 10);
    sprintf(str, "test_%d\n", 10);
    write(STDOUT_FILENO, str, strlen(str));
    str = "end";
}

void test_kbd() {
    fd_t fd = open("/dev/tty0", 1 << 0 | 1 << 10 | 1 << 11 | 1 << 12);
    dup2(STDIN_FILENO, fd);
    dup2(STDOUT_FILENO, fd);
    dup2(STDERR_FILENO, fd);
    int buf_size = 32;
    char *buf = mmap_anonymous(buf_size);
    ssize_t count = 0;
    size_t get_size = 0;
    while ((count = read(STDIN_FILENO, (void *)buf, buf_size)) > 0) {
        get_size = count;
    }
}
void test_device() {
    pid_t pid = fork();
    if (pid == 0) {
        // test_tty();
        test_kbd();
    } else {
    }
}
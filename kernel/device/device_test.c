#include "../syscall/syscall_user.h"
#include "common/lib/string.h"
#include "common/lib/stdio.h"

void test_tty() {
    char *str = "test_stdout\n";
    ssize_t count =  write(STDOUT_FILENO, str, strlen(str));
    str = "test_stderr\n";
    count = write(STDERR_FILENO, str, strlen(str));
    str = mmap_anonymous(10);
    memset(str, 0, 10);
    sprintf(str, "test_%d\n", 10);
    write(STDOUT_FILENO, str, strlen(str));
    str = "end";
}
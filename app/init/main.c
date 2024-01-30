#include "common/tool/lib.h"
#include "include/syscall.h"
void init_shell() {
    const char *file_path = "/ext2/shell";
    execve(file_path, NULL, NULL);
}
void init_process() {
    // test_fork();
    pid_t pid = get_pid();
    ASSERT(pid == 1);
    sys_info info;
    sysinfo(pid, &info, SYS_INFO_MEM);
    // test_clone();
    // test_malloc_process();
    pid = fork();
    if (pid == 0) {
        // test_device();
        init_shell();
        return;
    }
    for (;;) {
        int status;
        pid_t child_pid = wait(&status);
        sysinfo(pid, &info, SYS_INFO_MEM);
        pid_t cpid = child_pid;
        if (child_pid == -1) {
            yield();
        }
    }
}

int main(int argc, char **argv) {
    init_process();
    return 0;
}
#include "./interrupt/intr.h"
#include "./syscall/syscall_user.h"
#include "./thread/process.h"
#include "./thread/thread.h"
#include "common/cpu/contrl.h"
#include "common/interrupt/memory_info.h"
#include "common/tool/lib.h"
#include "common/tool/log.h"
#include "common/types/basic.h"
#include "fs/fs.h"

extern void test_kernel_init();

void kernel_init(struct boot_params *params) {
    // test_kernel_init();

    init_mem(params);
    init_int();
    init_log();
    init_task();
    init_fs();
}

extern void test_fork();
extern void test_clone();
extern void test_malloc_process();
extern void test_device();

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

extern void test_main();

void idle(void *arg) {
    for (;;) {
        sti();
        hlt();
    }
}

void main() {
    log_debug("enter kernel main\n");
    enter_main_thread();
    thread_start("idle", 1, idle, "");
    // test_main();
    process_execute(init_process, "init");
    int count = 0;
    /*
    for (;;) {
        count++;
        uint32_t pid = get_pid();
        // int a = 10 / 0;
        hlt();
    }*/
    exit(0);
}
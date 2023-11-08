#include "./interrupt/intr.h"
#include "./syscall/syscall_user.h"
#include "./thread/process.h"
#include "./thread/thread.h"
#include "common/cpu/contrl.h"
#include "common/interrupt/memory_info.h"
#include "common/types/basic.h"

extern void test_kernel_init();

void kernel_init(struct boot_params *params) {
    // test_kernel_init();

    init_mem(params);
    init_int();
    init_task();
}

extern void test_fork();
extern void test_clone();
extern void test_malloc_process();

void init_process() {
    // test_fork();
    sys_info info;
    pid_t pid = get_pid();
    sysinfo(pid, &info);
    // test_clone();
    test_malloc_process();
    for (;;) {
        int status;
        uint32_t child_pid = wait(&status);
        sysinfo(pid, &info);
        uint32_t cpid = child_pid;
        if (child_pid == -1) {
            yield();
        }
    }
}

extern void test_main();

void idle(void *arg) {
    for (;;) {
        ps_info pss[10];
        size_t count = ps(pss, 10);
        if (count > 0) {
            for (int i = 0; i < count; i++) {
                
            }
        }
        sti();
        hlt();
    }
}

void main() {
    enter_main_thread();
    thread_start("idle", 1, idle, "");
    // test_main();
    process_execute(init_process, "init");
    int count = 0;
    for (;;) {
        count++;
        uint32_t pid = get_pid();
        // int a = 10 / 0;
        hlt();
    }
}
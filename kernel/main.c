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

extern void test_main();

extern void process_execute_init();
void idle(void *arg) {
    process_execute_init();
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
    exit(0);
}
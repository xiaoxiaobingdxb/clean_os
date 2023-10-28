#include "common/types/basic.h"
#include "common/interrupt/memory_info.h"
#include "common/cpu/contrl.h"
#include "./thread/thread.h"
#include "./thread/process.h"
#include "./syscall/syscall_user.h"
#include "./interrupt/intr.h"

extern void test_kernel_init();

void kernel_init(struct boot_params *params) {
    // test_kernel_init();

    init_mem(params);
    init_int();
    init_task();
}

void init_process() {
    int count = 0;
    uint32_t child_pid = fork();
    if (!child_pid) { // is 
        for (;;) {
            count += 2;
            uint32_t pid = get_pid();
            // int a = 10 / 0;
            yield();
        }
    }
    for (;;) {
        count += 1;
        uint32_t pid = get_pid();
        // int a = 10 / 0;
        yield();
    }
}

extern void test_main();

void main() {
    enter_main_thread();
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
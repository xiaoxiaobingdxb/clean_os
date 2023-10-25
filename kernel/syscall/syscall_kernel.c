#include "syscall_kernel.h"

#include "../interrupt/idt.h"
#include "../thread/process.h"
#include "../thread/thread.h"
#include "syscall_no.h"

#define SYSCALL_SIZE 512
#define SYSCALL_INTR_NO 0x80
typedef void* syscall;
syscall syscall_table[SYSCALL_SIZE];

#define sycall_register(number, entry_point) syscall_table[number] = entry_point

extern void intr_entry_syscall();
void init_syscall() {
    sycall_register(SYSCALL_get_pid, process_get_pid);
    sycall_register(SYSCALL_sched_yield, thread_yield);
    sycall_register(SYSCALL_execv, process_execve);
    make_intr(SYSCALL_INTR_NO, IDT_DESC_ATTR_DPL3, intr_entry_syscall);
}
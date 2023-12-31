#include "syscall_kernel.h"

#include "../interrupt/idt.h"
#include "../memory/mem.h"
#include "../thread/process.h"
#include "../thread/thread.h"
#include "syscall_no.h"

#define SYSCALL_SIZE 512
#define SYSCALL_INTR_NO 0x80
typedef void *syscall;
syscall syscall_table[SYSCALL_SIZE];

#define syscall_register(number, entry_point) \
    syscall_table[number] = entry_point

extern void intr_entry_syscall();

void *syscall_memory_mmap(void *addr, size_t length, int prot, int flags,
                          int fd) {
    return process_mmap(addr, length, prot, flags, fd, 0);
}

void init_syscall() {
    syscall_register(SYSCALL_get_pid, process_get_pid);
    syscall_register(SYSCALL_get_ppid, process_get_ppid);
    syscall_register(SYSCALL_sched_yield, thread_yield);
    syscall_register(SYSCALL_fork, process_fork);
    syscall_register(SYSCALL_wait, process_wait);
    syscall_register(SYSCALL_exit, process_exit);
    syscall_register(SYSCALL_execv, process_execve);
    syscall_register(SYSCALL_clone, process_clone);
    syscall_register(SYSCALL_mmap, syscall_memory_mmap);
    syscall_register(SYSCALL_sysinfo, process_sysinfo);
    syscall_register(SYSCALL_ps, process_ps);
    make_intr(SYSCALL_INTR_NO, IDT_DESC_ATTR_DPL3, intr_entry_syscall);
}
#include "syscall_kernel.h"

#include "../fs/fs.h"
#include "../interrupt/idt.h"
#include "../memory/mem.h"
#include "../thread/process.h"
#include "../thread/thread.h"
#include "common/tool/math.h"
#include "syscall_no.h"
#include "../time/time.h"

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

void syscall_memory_munmap(void *addr, size_t length) {
    int page_count = up2(length, MEM_PAGE_SIZE) / MEM_PAGE_SIZE;
    unmalloc_thread_mem((uint32_t)addr, page_count);
}

void init_syscall() {
    syscall_register(SYSCALL_mmap, syscall_memory_mmap);
    syscall_register(SYSCALL_munmap, syscall_memory_munmap);

    syscall_register(SYSCALL_get_pid, process_get_pid);
    syscall_register(SYSCALL_get_ppid, process_get_ppid);
    syscall_register(SYSCALL_sched_yield, thread_yield);
    syscall_register(SYSCALL_fork, process_fork);
    syscall_register(SYSCALL_wait, process_wait);
    syscall_register(SYSCALL_exit, process_exit);
    syscall_register(SYSCALL_execv, process_execve);
    syscall_register(SYSCALL_clone, process_clone);
    syscall_register(SYSCALL_sysinfo, process_sysinfo);
    syscall_register(SYSCALL_ps, process_ps);

    syscall_register(SYSCALL_open, sys_open);
    syscall_register(SYSCALL_dup, sys_dup);
    syscall_register(SYSCALL_dup2, sys_dup2);
    syscall_register(SYSCALL_write, sys_write);
    syscall_register(SYSCALL_read, sys_read);
    syscall_register(SYSCALL_lseek, sys_lseek);
    syscall_register(SYSCALL_close, sys_close);
    syscall_register(SYSCALL_stat, sys_stat);
    syscall_register(SYSCALL_fstat, sys_fstat);
    syscall_register(SYSCALL_readdir, sys_readdir);
    syscall_register(SYSCALL_mkdir, sys_mkdir);
    syscall_register(SYSCALL_rmdir, sys_rmdir);
    syscall_register(SYSCALL_link, sys_link);
    syscall_register(SYSCALL_symlink, sys_symlink);
    syscall_register(SYSCALL_unlink, sys_link);

    syscall_register(SYSCALL_timestamp, sys_timestamp);

    make_intr(SYSCALL_INTR_NO, IDT_DESC_ATTR_DPL3, intr_entry_syscall);
}
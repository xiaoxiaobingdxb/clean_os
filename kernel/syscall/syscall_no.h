#ifndef SYSCALL_SYSCALL_NO_H
#define SYSCALL_SYSCALL_NO_H

enum syscall_no_ {

    SYSCALL_get_pid,
    SYSCALL_get_ppid,
    SYSCALL_sched_yield,
    SYSCALL_fork,
    SYSCALL_wait,
    SYSCALL_exit,
    SYSCALL_execv,
    SYSCALL_clone,
    SYSCALL_mmap,
    SYSCALL_munmap,
    SYSCALL_sysinfo,
    SYSCALL_ps,
    SYSCALL_open,
    SYSCALL_write,
    SYSCALL_read,
    SYSCALL_lseek,
    SYSCALL_close,
    SYSCALL_stat,
    SYSCALL_fstat,
    SYSCALL_dup,
    SYSCALL_dup2,
};

#endif
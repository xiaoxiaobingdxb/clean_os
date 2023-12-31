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
    SYSCALL_sysinfo,
    SYSCALL_ps,
};

#endif
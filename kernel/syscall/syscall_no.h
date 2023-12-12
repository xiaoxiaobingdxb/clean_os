#ifndef SYSCALL_SYSCALL_NO_H
#define SYSCALL_SYSCALL_NO_H

enum syscall_no_ {

    // memory
    SYSCALL_mmap,
    SYSCALL_munmap,

    // process
    SYSCALL_get_pid,
    SYSCALL_get_ppid,
    SYSCALL_sched_yield,
    SYSCALL_fork,
    SYSCALL_wait,
    SYSCALL_exit,
    SYSCALL_execv,
    SYSCALL_clone,
    SYSCALL_sysinfo,
    SYSCALL_ps,

    // fs
    SYSCALL_open,
    SYSCALL_dup,
    SYSCALL_dup2,
    SYSCALL_write,
    SYSCALL_read,
    SYSCALL_lseek,
    SYSCALL_close,
    SYSCALL_stat,
    SYSCALL_fstat,
    SYSCALL_readdir,
    SYSCALL_mkdir,
    SYSCALL_rmdir,
    SYSCALL_link,
    SYSCALL_symlink,
    SYSCALL_unlink,
};

#endif
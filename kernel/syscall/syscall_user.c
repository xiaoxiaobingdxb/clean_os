#include "syscall_user.h"

#include "syscall_no.h"

/**
 * @see ./syscall_kernel.c
 */
#define syscall0(syscall_no)                                           \
    ({                                                                 \
        uint32_t ret_val;                                              \
        asm("int $0x80" : "=a"(ret_val) : "a"(syscall_no) : "memory"); \
        ret_val;                                                       \
    })

#define syscall1(syscall_no, arg0)       \
    ({                                   \
        uint32_t ret_val;                \
        asm("int $0x80"                  \
            : "=a"(ret_val)              \
            : "a"(syscall_no), "b"(arg0) \
            : "memory");                 \
        ret_val;                         \
    })
#define syscall2(syscall_no, arg0, arg1)            \
    ({                                              \
        uint32_t ret_val;                           \
        asm("int $0x80"                             \
            : "=a"(ret_val)                         \
            : "a"(syscall_no), "b"(arg0), "c"(arg1) \
            : "memory");                            \
        ret_val;                                    \
    })
#define syscall3(syscall_no, arg0, arg1, arg2)                 \
    ({                                                         \
        uint32_t ret_val;                                      \
        asm("int $0x80"                                        \
            : "=a"(ret_val)                                    \
            : "a"(syscall_no), "b"(arg0), "c"(arg1), "d"(arg2) \
            : "memory");                                       \
        ret_val;                                               \
    })

#define syscall4(syscall_no, arg0, arg1, arg2, arg3)                      \
    ({                                                                    \
        uint32_t ret_val;                                                 \
        asm("int $0x80"                                                   \
            : "=a"(ret_val)                                               \
            : "a"(syscall_no), "b"(arg0), "c"(arg1), "d"(arg2), "S"(arg3) \
            : "memory");                                                  \
        ret_val;                                                          \
    })

#define syscall6(syscall_no, arg0, arg1, arg2, arg3, arg4, arg5)           \
    ({                                                                     \
        uint32_t ret_val;                                                  \
        asm("push %%ebp\n\t"                                               \
            "push %[_arg5]\n\t"                                            \
            "mov %%esp, %%ebp\n\t"                                         \
            "int $0x80\n\t"                                                \
            "add $0x4, %%esp\n\t"                                          \
            "pop %%ebp"                                                    \
            : "=a"(ret_val)                                                \
            : "a"(syscall_no), "b"(arg0), "c"(arg1), "d"(arg2), "S"(arg3), \
              "D"(arg4), [_arg5] "m"(arg5)                                 \
            : "memory");                                                   \
        ret_val;                                                           \
    })
void yield() { syscall0(SYSCALL_sched_yield); }

pid_t get_pid() { return syscall0(SYSCALL_get_pid); }

pid_t get_ppid() { return syscall0(SYSCALL_get_ppid); }

pid_t fork() { return syscall0(SYSCALL_fork); }

pid_t wait(int *status) { return syscall1(SYSCALL_wait, status); }

void exit(int status) { syscall0(SYSCALL_exit); }

int execve(const char *filename, char *const argv[], char *const envp[]) {
    return syscall3(SYSCALL_execv, filename, argv, envp);
}

void *mmap(void *addr, size_t length, int prot, int flags, int fd, int offset) {
    return (void *)syscall6(SYSCALL_mmap, addr, length, prot, flags, fd,
                            offset);
}

uint32_t clone(int (*func)(void *), void *child_stack, int flags,
               void *func_arg) {
    syscall4(SYSCALL_clone, func, child_stack, flags, func_arg);
}

void deprecated_clone(const char *name, uint32_t priority, void *func,
                      void *func_arg) {}

int sysinfo(uint32_t pid, sys_info *info) {
    return syscall2(SYSCALL_sysinfo, pid, info);
}

int ps(ps_info* ps, size_t count) {
    return syscall2(SYSCALL_ps, ps, count);
}
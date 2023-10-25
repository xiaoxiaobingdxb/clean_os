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

#define syscall3(syscall_no, arg0, arg1, arg2)                 \
    ({                                                         \
        uint32_t ret_val;                                      \
        asm("int $0x80"                                        \
            : "=a"(ret_val)                                    \
            : "a"(syscall_no), "b"(arg0), "c"(arg1), "d"(arg2) \
            : "memory");                                       \
        ret_val;                                               \
    })
void yield() { syscall0(SYSCALL_sched_yield); }

uint32_t get_pid() { return syscall0(SYSCALL_get_pid); }

int execve(const char *filename, char *const argv[], char *const envp[]) {
    return syscall3(SYSCALL_execv, filename, argv, envp);
}
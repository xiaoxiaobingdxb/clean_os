//
// Created by ByteDance on 2025/1/22.
//

#ifndef SYSTEMCALL_BASE_H
#define SYSTEMCALL_BASE_H


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

#endif //SYSTEMCALL_BASE_H

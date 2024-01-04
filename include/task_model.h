#if !defined(TASK_MODEL_H)
#define TASK_MODEL_H
#include "common/types/basic.h"

typedef uint32_t pid_t;

typedef struct {
    uint64_t rdtscp;
} cpu_info;

typedef struct {
    uint32_t kernel_mem_used;
    uint32_t user_mem_used;
    uint32_t kernel_phy_mem_used;
    uint32_t user_phy_mem_used;
    char pwd[64];
    cpu_info cpu_info;
} sys_info;

typedef struct {
    pid_t pid;
    pid_t ppid;
    const char name[16];
    int status;
} ps_info;

#endif  // TASK_MODEL_H

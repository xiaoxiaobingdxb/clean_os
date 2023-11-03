#ifndef THREAD_MODEL_H
#define THREAD_MODEL_H

#include "./type.h"
typedef struct {
    uint32_t kernel_mem_used;
    uint32_t user_mem_used;
    uint32_t kernel_phy_mem_used;
    uint32_t user_phy_mem_used;
} sys_info;

typedef struct {
    pid_t pid;
    pid_t ppid;
    const char name[TASK_NAME_LEN];
    task_status status;
} ps_info;

#endif
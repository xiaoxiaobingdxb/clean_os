#ifndef THREAD_MODEL_H
#define THREAD_MODEL_H
typedef struct {
    uint32_t kernel_mem_used;
    uint32_t user_mem_used;
    uint32_t kernel_phy_mem_used;
    uint32_t user_phy_mem_used;
} sys_info;
#endif
#ifndef THREAD_THREAD_H
#define THREAD_THREAD_H

#include "common/types/basic.h"
#include "common/lib/list.h"

#define TASK_NAME_LEN 16

typedef enum {
    TASK_RUNNING,
    TASK_READY,
    TASK_BLOCKED,
    TASK_WAITING,
    TASK_HANGING,
    TASK_DIED
} task_status;

typedef void thread_func(void *);

typedef struct {
    uint32_t ebp, ebx, edi, esi;
    void (*eip)(thread_func *func, void *func_arg);
    void(*unused_readdr);
    thread_func *func;
    void *func_arg;
} thread_stack;

typedef struct {
    uint32_t *self_kstack;
    char name[TASK_NAME_LEN];
    task_status status;
    list_node general_tag, all_tag;
    uint32_t priority;
    uint32_t ticks; // time running at cpu
    uint32_t elapset_ticks; // time running at cpu all life
} task_struct;

task_struct *cur_thread();

task_struct *thread_start(const char*name, uint32_t priority, thread_func func, void *func_arg);
task_struct *thread_start1(const char *name, thread_func func, void *func_arg);

extern void switch_to(task_struct *cur, task_struct *next);

void schedule();

void init_task();

void enter_main_thread();

#endif
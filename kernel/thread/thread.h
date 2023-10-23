#ifndef THREAD_THREAD_H
#define THREAD_THREAD_H

#include "common/types/basic.h"
#include "common/lib/list.h"
#include "../memory/mem.h"

#define TASK_NAME_LEN 16
#define TASK_DEFAULT_PRIORITY 31

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
    void(*unused_retaddr);
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

    uint32_t page_dir;

    vir_addr_alloc_t vir_addr_alloc;
} task_struct;

extern list ready_tasks;
extern list all_tasks;

task_struct *cur_thread();

void init_thread(task_struct *pthread, const char *name, uint32_t priority);
void thread_create(task_struct *pthread, thread_func func, void *func_arg);

task_struct *thread_start(const char*name, uint32_t priority, thread_func func, void *func_arg);
task_struct *thread_start1(const char *name, thread_func func, void *func_arg);

extern void switch_to(task_struct *cur, task_struct *next);

void schedule();

void init_task();

void enter_main_thread();

uint32_t malloc_thread_mem(int page_count);
uint32_t malloc_thread_mem_vaddr(uint32_t vaddr, int page_count);
int unmalloc_thread_mem(uint32_t vaddr, int page_count);

void yield();

#endif
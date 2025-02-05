#ifndef THREAD_THREAD_H
#define THREAD_THREAD_H

#include "../fs/file.h"
#include "../memory/mem.h"
#include "./schedule/completion.h"
#include "./type.h"
#include "common/lib/list.h"
#include "common/types/basic.h"

#define TASK_DEFAULT_PRIORITY 31
#define TASK_FILE_SIZE 128

typedef void thread_func(void *p_func);

typedef struct {
    uint32_t ebp, ebx, edi, esi;
    void (*eip)(thread_func *func, void *func_arg);
    void(*unused_retaddr);
    thread_func *func;
    void *func_arg;
} thread_stack;

typedef struct {
    uint32_t *self_kstack;
    pid_t pid, parent_pid;
    char name[TASK_NAME_LEN];
    task_status status;
    int exit_code;
    list_node general_tag, waiter_tag, all_tag;
    uint32_t priority;
    uint32_t ticks;          // time running at cpu
    uint32_t elapset_ticks;  // time running at cpu all life
    uint32_t sleep_ticks;
    completion *vfork_done;

    uint32_t page_dir;

    vir_addr_alloc_t vir_addr_alloc;

    file_t *file_table[TASK_FILE_SIZE];
    char pwd[64];
} task_struct;

extern list ready_tasks;
extern list sleep_tasks;
extern list all_tasks;

task_struct *cur_thread();

void init_thread(task_struct *pthread, const char *name, uint32_t priority);
void thread_create(task_struct *pthread, thread_func func, void *func_arg);

task_struct *thread_start(const char *name, uint32_t priority, thread_func func,
                          void *func_arg);
task_struct *thread_start1(const char *name, thread_func func, void *func_arg);

extern void switch_to(task_struct *cur, task_struct *next);

void schedule();

void init_task();

void enter_main_thread();

typedef enum {
    PF_KERNEL,
    PF_USER,
} page_flag;
uint32_t malloc_thread_mem(page_flag pf, int page_count);
uint32_t malloc_thread_mem_vaddr(uint32_t vaddr, int page_count);
int unmalloc_thread_mem(uint32_t vaddr, int page_count);

void thread_yield();
void thread_sleep(uint32_t ms_n);
void thread_clone(const char *name, uint32_t priority, thread_func func,
                  void *func_arg);

void thread_block(task_struct *task, task_status status);
void thread_ready(task_struct *task, bool need_schedule, bool now);
void thread_exit(task_struct *task, bool need_shedule);

void set_thread_status(task_struct *task, task_status status,
                       bool need_schedule);
bool has_other_thread(task_struct *task);

task_struct *pid2task(pid_t pid);
task_struct *thread_child(task_struct *parent, task_status status);

fd_t task_alloc_fd(file_t *file);
int task_set_file(fd_t fd, file_t *file, bool override);
file_t *task_file(fd_t fd);
void task_free_fd(fd_t fd);

#endif
#include "thread.h"

#include "../memory/mem.h"
#include "common/cpu/mem_page.h"
#include "common/lib/list.h"
#include "common/lib/string.h"
#include "timer.h"
#include "common/cpu/contrl.h"

list ready_tasks;
list all_tasks;

static void kernel_thread(thread_func *func, void *func_arg) {
    sti();
    func(func_arg);
}

void thread_create(task_struct *pthread, thread_func func, void *func_arg) {
    pthread->self_kstack -= sizeof(thread_stack);
    thread_stack *kthread_stack = (thread_stack *)pthread->self_kstack;
    kthread_stack->ebp = kthread_stack->ebx;
    kthread_stack->esi = kthread_stack->edi;

    kthread_stack->eip = kernel_thread;

    kthread_stack->func = func;
    kthread_stack->func_arg = func_arg;
}

task_struct *main_thread;

void init_thread(task_struct *pthread, const char *name, uint32_t priority) {
    memset(pthread, 0, sizeof(*pthread));
    memcpy(pthread->name, name, strlen(name));
    if (pthread == main_thread) {
        pthread->status = TASK_RUNNING;
    } else {
        pthread->status = TASK_READY;
    }
    pthread->self_kstack = (uint32_t *)((uint32_t)pthread + MEM_PAGE_SIZE);
    pthread->priority = priority;
    pthread->ticks = priority;
    pthread->elapset_ticks = 0;
}

task_struct *cur_thread() {
    uint32_t esp;
    __asm__ __volatile__("mov %%esp, %[v]" : [v] "=r"(esp));
    return (task_struct *)(esp & 0xfffff000);
}

task_struct *thread_start(const char *name, uint32_t priority, thread_func func, void *func_arg) {
    task_struct *thread = (task_struct *)alloc_kernel_mem(1);
    init_thread(thread, name, priority);
    thread_create(thread, func, func_arg);
    pushr(&ready_tasks, &thread->general_tag);
    pushr(&all_tasks, &thread->all_tag);
    return thread;
}

task_struct *thread_start1(const char *name, thread_func func, void *func_arg) {
    task_struct *thread = (task_struct *)alloc_kernel_mem(1);
    init_thread(thread, name, 31);
    thread_create(thread, func, func_arg);
    __asm__ __volatile__(
        "mov %[v], %%esp; pop %%ebp; pop %%ebx; pop %%edi; pop %%esi; ret" ::
            [v] "r"(thread->self_kstack));
    return thread;
}

void schedule() {
    task_struct *cur = cur_thread();
    cur->elapset_ticks++;
    if (cur->ticks-- > 0) { // continue run current task
        return;
    }
    list_node *next_tag = popl(&ready_tasks);
    task_struct *next = tag2entry(task_struct, general_tag, next_tag);
    if (next == NULL) {
        return;
    }
    next->ticks = next->priority;
    if (cur == next) {
        return;
    }
    cur->status = TASK_READY;
    next->status = TASK_RUNNING;
    pushr(&ready_tasks, &cur->general_tag);
    switch_to(cur, next);
}

void enter_main_thread() {
    main_thread = cur_thread();
    init_thread(main_thread, "main", 31);
    pushr(&all_tasks, &main_thread->all_tag);
}

void init_task() {
    init_timer();
    init_list(&ready_tasks);
    init_list(&all_tasks);
}
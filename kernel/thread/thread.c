#include "thread.h"

#include "../memory/mem.h"
#include "common/cpu/mem_page.h"
#include "common/lib/string.h"

static void kernel_thread(thread_func *func, void *func_arg) { func(func_arg); }

void thread_create(task_struct *pthread, thread_func func, void *func_arg) {
    pthread->self_kstack -= sizeof(thread_stack);
    thread_stack *kthread_stack = (thread_stack *)pthread->self_kstack;
    kthread_stack->ebp = kthread_stack->ebx;
    kthread_stack->esi = kthread_stack->edi;

    kthread_stack->eip = kernel_thread;

    kthread_stack->func = func;
    kthread_stack->func_arg = func_arg;
}

void init_thread(task_struct *pthread, const char *name) {
    memset(pthread, 0, sizeof(*pthread));
    memcpy(pthread->name, name, strlen(name));
    pthread->status = TASK_RUNNING;
    pthread->self_kstack = (uint32_t *)((uint32_t)pthread + MEM_PAGE_SIZE);
}

task_struct *cur_thread() {
    uint32_t esp;
    __asm__ __volatile__("mov %%esp, %[v]" : [v] "=r"(esp));
    return (task_struct *)(esp & 0xfffff000);
}

task_struct *thread_start(const char *name, thread_func func, void *func_arg) {
    task_struct *thread = (task_struct *)alloc_kernel_mem(1);
    init_thread(thread, name);
    thread_create(thread, func, func_arg);
    return thread;
}

task_struct *thread_start1(const char *name, thread_func func, void *func_arg) {
    task_struct *thread = (task_struct *)alloc_kernel_mem(1);
    init_thread(thread, name);
    thread_create(thread, func, func_arg);
    __asm__ __volatile__(
        "mov %[v], %%esp; pop %%ebp; pop %%ebx; pop %%edi; pop %%esi; ret" ::
            [v] "r"(thread->self_kstack));
    return thread;
}
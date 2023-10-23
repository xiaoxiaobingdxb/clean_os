#include "thread.h"

#include "../memory/mem.h"
#include "common/cpu/contrl.h"
#include "common/cpu/mem_page.h"
#include "common/lib/list.h"
#include "common/lib/string.h"
#include "timer.h"
#include "process.h"

list ready_tasks;
list all_tasks;

static void kernel_thread(thread_func *func, void *func_arg) {
    sti();
    func(func_arg);
}

void thread_create(task_struct *pthread, thread_func func, void *func_arg) {
    pthread->self_kstack -= sizeof(process_stack);
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

task_struct *thread_start(const char *name, uint32_t priority, thread_func func,
                          void *func_arg) {
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

void process_activate(task_struct *thread) {
    uint32_t page_dir_phy_addr = thread->page_dir;
    set_page_dir(page_dir_phy_addr);
    update_tss_esp((uint32_t)thread + MEM_PAGE_SIZE);
}

void schedule() {
    task_struct *cur = cur_thread();
    if (cur->status == TASK_RUNNING) {
        cur->elapset_ticks++;
        if (cur->ticks-- > 0) {  // continue run current task
            return;
        }
    }
    list_node *next_tag = popl(&ready_tasks);
    if (next_tag == NULL) {
        return;
    }
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
    process_activate(next);
    switch_to(cur, next);
}

void init_task() {
    init_timer();
    init_list(&ready_tasks);
    init_list(&all_tasks);
}

extern pde_t kernel_page_dir[PDE_CNT] __attribute__((aligned(MEM_PAGE_SIZE)));
void enter_main_thread() {
    main_thread = cur_thread();
    init_thread(main_thread, "main", TASK_DEFAULT_PRIORITY);
    main_thread->page_dir = (uint32_t)kernel_page_dir;
    extern vir_addr_alloc_t kernel_vir_addr_alloc;
    memcpy(&main_thread->vir_addr_alloc, &kernel_vir_addr_alloc, sizeof(kernel_vir_addr_alloc));
    pushr(&all_tasks, &main_thread->all_tag);
}

uint32_t malloc_thread_mem(int page_count) {
    task_struct *cur = cur_thread();
    if (cur == NULL) {
        return 0;
    }
    return malloc_mem(cur->page_dir, &cur->vir_addr_alloc, page_count);
}

uint32_t malloc_thread_mem_vaddr(uint32_t vaddr, int page_count) {
    task_struct *cur = cur_thread();
    if (cur == NULL) {
        return 0;
    }
    return malloc_mem_vaddr(cur->page_dir, &cur->vir_addr_alloc, vaddr, page_count);
}

int unmalloc_thread_mem(uint32_t vaddr, int page_count) {
    task_struct *cur = cur_thread();
    if (cur == NULL) {
        return 0;
    }
    return unmalloc_mem(cur->page_dir, &cur->vir_addr_alloc, vaddr, page_count);
}

void yield() {
    task_struct *cur = cur_thread();
    cur->status = TASK_READY;
    schedule();
}
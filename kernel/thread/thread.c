#include "thread.h"

#include "../memory/mem.h"
#include "common/cpu/contrl.h"
#include "common/cpu/mem_page.h"
#include "common/lib/list.h"
#include "common/lib/string.h"
#include "common/tool/lib.h"
#include "process.h"
#include "timer.h"

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

typedef struct {
    bitmap_t bitmap;
    uint32_t start;
} pid_alloc_t;
uint8_t pid_bitmap_bits[128];
pid_alloc_t pid_alloc;
void init_pid_alloc() {
    pid_alloc.bitmap.bits = pid_bitmap_bits;
    pid_alloc.bitmap.bytes_len = sizeof(pid_bitmap_bits);
    pid_alloc.start = 0;
    bitmap_init(&pid_alloc.bitmap);
}
pid_t alloc_pid() {
    int bit_idx = bitmap_scan(&pid_alloc.bitmap, 1);
    if (bit_idx < 0) {
        return -1;
    }
    bitmap_set(&pid_alloc.bitmap, bit_idx, 1);
    return bit_idx + pid_alloc.start;
}

void unalloc_pid(pid_t pid) {
    int bit_idx = pid - pid_alloc.start;
    bitmap_set(&pid_alloc.bitmap, bit_idx, 0);
}

void init_thread(task_struct *pthread, const char *name, uint32_t priority) {
    memset(pthread, 0, sizeof(task_struct));
    memcpy(pthread->name, name, strlen(name));
    if (pthread == main_thread) {
        pthread->status = TASK_RUNNING;
    } else {
        pthread->status = TASK_READY;
    }
    pthread->self_kstack = (uint32_t *)((uint32_t)pthread + MEM_PAGE_SIZE);
    pthread->general_tag.pre = NULL;
    pthread->general_tag.next = NULL;
    pthread->all_tag.pre = NULL;
    pthread->all_tag.next = NULL;
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
    task_struct *cur = cur_thread();
    task_struct *thread = (task_struct *)alloc_kernel_mem(1);
    init_thread(thread, name, priority);
    thread->pid = cur->pid;
    thread->parent_pid = cur->parent_pid;
    thread_create(thread, func, func_arg);
    // all threads share virtual memory in a process
    // so, copy page_dir and vir_addr_alloc
    thread->page_dir = cur->page_dir;
    thread->vir_addr_alloc.start = cur->vir_addr_alloc.start;
    thread->vir_addr_alloc.page_size = cur->vir_addr_alloc.page_size;
    thread->vir_addr_alloc.bitmap = cur->vir_addr_alloc.bitmap;

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
    if (cur->status == TASK_RUNNING && cur->ticks > 0) {
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
    // only when cur task status is running, push cur into ready task queue
    if (cur->status == TASK_RUNNING) {
        cur->status = TASK_READY;
    }
    if (cur->status == TASK_READY) {
        pushr(&ready_tasks, &cur->general_tag);
    }
    next->status = TASK_RUNNING;

    process_activate(next);
    switch_to(cur, next);
}

void init_task() {
    init_timer();
    init_list(&ready_tasks);
    init_list(&all_tasks);
    init_pid_alloc();
}

extern pde_t kernel_page_dir[PDE_CNT] __attribute__((aligned(MEM_PAGE_SIZE)));
extern vir_addr_alloc_t kernel_vir_addr_alloc;
void enter_main_thread() {
    main_thread = cur_thread();
    init_thread(main_thread, "main", TASK_DEFAULT_PRIORITY);
    main_thread->page_dir = (uint32_t)kernel_page_dir;
    main_thread->pid = alloc_pid();
    main_thread->parent_pid = -1;
    memcpy(&main_thread->vir_addr_alloc, &kernel_vir_addr_alloc,
           sizeof(kernel_vir_addr_alloc));
    pushr(&all_tasks, &main_thread->all_tag);
}

uint32_t malloc_thread_mem(page_flag pf, int page_count) {
    uint32_t page_dir;
    vir_addr_alloc_t *vir_addr_alloc;
    if (pf == PF_KERNEL) {
        page_dir = (uint32_t)kernel_page_dir;
        vir_addr_alloc = &kernel_vir_addr_alloc;
    } else {
        task_struct *cur = cur_thread();
        if (cur == NULL) {
            return -1;
        }
        page_dir = cur->page_dir;
        vir_addr_alloc = &cur->vir_addr_alloc;
    }

    return malloc_mem(page_dir, vir_addr_alloc, page_count);
}

uint32_t malloc_thread_mem_vaddr(uint32_t vaddr, int page_count) {
    task_struct *cur = cur_thread();
    if (cur == NULL) {
        return 0;
    }
    uint32_t page_dir;
    vir_addr_alloc_t *vir_addr_alloc;
    if (vaddr >= cur->vir_addr_alloc.start) {
        page_dir = cur->page_dir;
        vir_addr_alloc = &cur->vir_addr_alloc;
    } else {
        page_dir = (uint32_t)kernel_page_dir;
        vir_addr_alloc = &kernel_vir_addr_alloc;
    }
    return malloc_mem_vaddr(page_dir, vir_addr_alloc, vaddr, page_count);
}

int unmalloc_thread_mem(uint32_t vaddr, int page_count) {
    task_struct *cur = cur_thread();
    if (cur == NULL) {
        return -1;
    }
    uint32_t page_dir;
    vir_addr_alloc_t *vir_addr_alloc;
    if (vaddr >= cur->vir_addr_alloc.start) {
        page_dir = cur->page_dir;
        vir_addr_alloc = &cur->vir_addr_alloc;
    } else {
        page_dir = (uint32_t)kernel_page_dir;
        vir_addr_alloc = &kernel_vir_addr_alloc;
    }
    return unmalloc_mem(page_dir, vir_addr_alloc, vaddr, page_count);
}

void _set_thread_status(task_struct *task, task_status status,
                        bool need_schedule, bool first) {
    if (task->status != TASK_READY && task->status != TASK_RUNNING &&
        status == TASK_READY) {
        if (first) {
            pushl(&ready_tasks, &task->general_tag);
        } else {
            pushr(&ready_tasks, &task->general_tag);
        }
    }
    if (status != TASK_READY) {
        remove(&ready_tasks, &task->general_tag);
    }
    task->status = status;
    if (need_schedule) {
        schedule();
    }
}

void set_thread_status(task_struct *task, task_status status,
                       bool need_schedule) {
    _set_thread_status(task, status, need_schedule, false);
}

void set_cur_thread_status(task_status status, bool need_schedule) {
    task_struct *cur = cur_thread();
    if (cur == NULL) {
        return;
    }
    set_thread_status(cur, status, need_schedule);
}

void thread_yield() { set_cur_thread_status(TASK_READY, true); }

void thread_clone(const char *name, uint32_t priority, thread_func func,
                  void *func_arg) {
    thread_start(name, priority, func, func_arg);
}

void thread_block(task_struct *task, task_status status) {
    set_thread_status(task, status, true);
}

void thread_ready(task_struct *task, bool need_schedule, bool now) {
    _set_thread_status(task, TASK_READY, need_schedule, now);
}

bool pid_find_in_all_tasks(list_node *node, void *arg) {
    task_struct **args = (task_struct **)arg;
    task_struct *task = tag2entry(task_struct, all_tag, node);
    if (args[0] != task && task->status != TASK_HANGING &&
        task->status != TASK_DIED && args[0]->page_dir == task->page_dir) {
        args[1] = task;
        return false;
    }
    return true;
}

bool has_other_thread(task_struct *task) {
    task_struct *args[2] = {task, NULL};
    foreach (&all_tasks, pid_find_in_all_tasks, (void *)args)
        ;
    return args[1] != NULL;
}

bool pid2task_visitor(list_node *node, void *arg) {
    task_struct *task = tag2entry(task_struct, all_tag, node);
    uint32_t *args = (uint32_t *)arg;
    if (task->pid == args[0]) {
        args[1] = (uint32_t)task;
        return false;
    }
    return true;
}

task_struct *pid2task(pid_t pid) {
    uint32_t arg[2] = {pid, 0};
    foreach (&all_tasks, pid2task_visitor, (void *)arg)
        ;
    return (task_struct *)arg[1];
}

bool thread_child_visitor(list_node *node, void *arg) {
    task_struct *task = tag2entry(task_struct, all_tag, node);
    uint32_t *args = (uint32_t *)arg;
    if (task->parent_pid == args[0] && ((uint32_t)TASK_UNKNOWN == args[1] ||
                                        (uint32_t)task->status == args[1])) {
        args[2] = (uint32_t)task;
        return false;
    }
    return true;
}

task_struct *thread_child(task_struct *parent, task_status status) {
    uint32_t args[3] = {parent->pid, (uint32_t)status, 0};
    foreach (&all_tasks, thread_child_visitor, (void *)args)
        ;
    return (task_struct *)args[2];
}

void thread_exit(task_struct *task, bool need_schedule) {
    set_thread_status(task, TASK_DIED, need_schedule);
    remove(&ready_tasks, &task->general_tag);
    remove(&all_tasks, &task->all_tag);
    // if task is not main task
    if (task != main_thread) {
        // not find any task in this process, should to release pde
        if (!has_other_thread(task)) {
            unmalloc_thread_mem((uint32_t)task->page_dir, 1);
        }
        // release thread kernel stack
        unmalloc_thread_mem((uint32_t)task, 1);
    }
    unalloc_pid(task->pid);
}

fd_t task_alloc_fd(file_t *file) {
    task_struct *cur = cur_thread();
    ASSERT(cur != NULL);
    for (int i = 0; i < TASK_FILE_SIZE; i++) {
        if (cur->file_table[i] == NULL) {
            cur->file_table[i] = file;
            return i;
        }
    }
    return -1;
}

int task_set_file(fd_t fd, file_t *file, bool override) {
    file_t *old = task_file(fd);
    if (old && !override) {
        return -1;
    }
    task_struct *cur = cur_thread();
    ASSERT(cur != NULL);
    cur->file_table[fd] = file;
    return 0;
}

file_t *task_file(fd_t fd) {
    task_struct *cur = cur_thread();
    ASSERT(cur != NULL);
    if (fd < 0 || fd >= TASK_FILE_SIZE) {
        return NULL;
    }
    return cur->file_table[fd];
}

void task_free_fd(fd_t fd) {
    task_struct *cur = cur_thread();
    ASSERT(cur != NULL);
    if (fd < 0 || fd >= TASK_FILE_SIZE) {
        return;
    }
    cur->file_table[fd] = NULL;
}
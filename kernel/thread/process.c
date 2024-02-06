#include "process.h"

#include "../memory/config.h"
#include "../memory/mem.h"
#include "../syscall/syscall_user.h"
#include "common/asm/tool.h"
#include "common/cpu/contrl.h"
#include "common/lib/stack.h"
#include "common/lib/stdio.h"
#include "common/lib/string.h"
#include "common/tool/log.h"
#include "common/tool/math.h"
#include "thread.h"

extern uint64_t kernel_all_memory, user_all_memory;
uint32_t user_mode_stack_addr() {
    uint32_t vaddr = (uint32_t)(kernel_all_memory + user_all_memory -
                                MEM_PAGE_SIZE);  // last 4K virtual memory
    return vaddr;
}

extern void intr_exit();

#define EFLAGS_IOPL_0 (0 << 12)  // IOPL0
#define EFLAGS_MBS (1 << 1)
#define EFLAGS_IF_1 (1 << 9)
/**
 * switch process from kernel mode into user mode
 */
void process_kernel2user(task_struct *thread, void *p_func, void *user_stack) {
    thread->self_kstack += sizeof(thread_stack);
    process_stack *p_stack = (process_stack *)thread->self_kstack;
    p_stack->edi = p_stack->esi = p_stack->ebp = p_stack->esp_dummy = 0;
    p_stack->ebx = p_stack->edx = p_stack->ecx = p_stack->eax = 0;
    p_stack->gs = 0;
    p_stack->gs = p_stack->fs = p_stack->es = p_stack->ds = USER_SELECTOR_DS;

    p_stack->err_code = 0;
    p_stack->eip = p_func;
    p_stack->cs = USER_SELECTOR_CS;
    p_stack->eflags = (EFLAGS_IOPL_0 | EFLAGS_MBS | EFLAGS_IF_1);

    p_stack->esp = user_stack;
    p_stack->ss = USER_SELECTOR_SS;
    asm("mov %[v], %%esp\n\t"
        "jmp intr_exit" ::[v] "g"(p_stack)
        : "memory");
}

void _start_process(void *p_func, int argc, char *const argv[]) {
    task_struct *thread = cur_thread();
    // alloc a page for user mode stack, and assign esp to to page end to be
    // stack top
    uint32_t vaddr = user_mode_stack_addr();
    bool have = false;
    vaddr2paddr(thread->page_dir, vaddr, &have);
    if (!have) {
        vaddr = malloc_thread_mem_vaddr(vaddr, USER_STACK_SIZE / MEM_PAGE_SIZE);
    }
    // memset((void *)vaddr, 0, MEM_PAGE_SIZE);
    void *user_stack = (void *)(vaddr + MEM_PAGE_SIZE);
    if (argc > 0 && argv != NULL) {
        user_stack = stack_push(user_stack, (void*)argv);
        user_stack = stack_push_value(user_stack, &argc, sizeof(int));
    }
    process_kernel2user(thread, p_func, user_stack);
}

void start_process(void *p_func) { _start_process(p_func, 0, NULL); }

// alloc page_dir for process and init virtual memory bitmap
void init_process_mem(task_struct *thread) {
    thread->page_dir = create_uvm();
    init_user_vir_addr_alloc(&thread->vir_addr_alloc);
}

void process_execute(void *p_func, const char *name) {
    task_struct *cur = cur_thread();
    if (cur == NULL) {
        return;
    }
    task_struct *thread = (task_struct *)alloc_kernel_mem(1);
    init_thread(thread, name, TASK_DEFAULT_PRIORITY);
    thread->pid = alloc_pid();
    thread->parent_pid = cur->pid;
    thread_create(thread, start_process, p_func);
    init_process_mem(thread);
    pushr(&ready_tasks, &thread->general_tag);
    pushr(&all_tasks, &thread->all_tag);
}


void start_init_process(void *p_func) {
    process_execve("/ext2/init", NULL, NULL);
}
void process_execute_init() {
    task_struct *cur = cur_thread();
    if (cur == NULL) {
        return;
    }
    task_struct *thread = (task_struct *)alloc_kernel_mem(1);
    init_thread(thread, "init", TASK_DEFAULT_PRIORITY);
    thread->pid = alloc_pid();
    thread->parent_pid = cur->pid;
    thread_create(thread, start_init_process, NULL);
    init_process_mem(thread);
    pushr(&ready_tasks, &thread->general_tag);
    pushr(&all_tasks, &thread->all_tag);
}

pid_t process_get_pid() {
    task_struct *cur = cur_thread();
    if (cur == NULL) {
        return -1;
    }
    return cur->pid;
}

pid_t process_get_ppid() {
    task_struct *cur = cur_thread();
    if (cur == NULL) {
        return -1;
    }
    return cur->parent_pid;
}

int copy_process_mem(task_struct *dest, task_struct *src) {
    // copy kenrel stack
    memcpy(dest, src, MEM_PAGE_SIZE);
    init_process_mem(dest);
    process_stack *p_stack = (process_stack *)((uint32_t)dest + MEM_PAGE_SIZE -
                                               sizeof(process_stack));
    // thread_stack only use ebp, ebx, edi, esi, eip
    thread_stack *kthread_stack = (thread_stack *)((uint32_t *)p_stack - 5);
    // child process will return from syscall_fork
    kthread_stack->eip = intr_exit;
    p_stack->eax = 0;  // child process return 0 at syscall fork
    dest->self_kstack = (uint32_t *)kthread_stack;

    // the buff to copy data from src into dest, after copy, should free
    void *buff = (void *)alloc_kernel_mem(1);

    // copy all user memory
    for (int idx_byte = 0; idx_byte < src->vir_addr_alloc.bitmap.bytes_len;
         idx_byte++) {
        if (src->vir_addr_alloc.bitmap
                .bits[idx_byte]) {  // use the byte in bitmap
            for (int idx_bit = idx_byte * 8; idx_bit < idx_byte * 8 + 8;
                 idx_bit++) {
                if (bitmap_scan_test(&src->vir_addr_alloc.bitmap, idx_bit)) {
                    uint32_t vaddr = src->vir_addr_alloc.start +
                                     idx_bit * src->vir_addr_alloc.page_size;
                    bitmap_set(&dest->vir_addr_alloc.bitmap, idx_bit, 1);
                    // copy a page from parent into buff
                    memcpy(buff, (void *)vaddr, src->vir_addr_alloc.page_size);
                    set_page_dir(dest->page_dir);
                    vaddr = malloc_mem_vaddr(dest->page_dir,
                                             &dest->vir_addr_alloc, vaddr, 1);
                    // copy a page from buff into child
                    memcpy((void *)vaddr, buff, dest->vir_addr_alloc.page_size);
                    set_page_dir(src->page_dir);
                }
            }
        }
    }

    unalloc_kernel_mem((uint32_t)buff, 1);
}

task_struct *copy_process() {
    task_struct *cur = cur_thread();
    if (!cur) {
        return NULL;
    }
    task_struct *thread = (task_struct *)alloc_kernel_mem(1);
    init_thread(thread, cur->name, cur->priority);
    int err = copy_process_mem(thread, cur);
    if (err) {
        return NULL;
    }
    thread->parent_pid = cur->pid;
    thread->pid = alloc_pid();
    return thread;
}

uint32_t process_fork() {
    task_struct *thread = copy_process();
    if (!thread) {
        return -1;
    }
    sprintf(thread->name, "%s_%d", "fork_from", thread->parent_pid);
    pushr(&ready_tasks, &thread->general_tag);
    pushr(&all_tasks, &thread->all_tag);
    return thread->pid;
}

uint32_t process_wait(int *status) {
    task_struct *cur = cur_thread();
    if (!cur) {
        return -1;
    }
    log_info("process_wait:%s\n", cur->name);
    for (;;) {
        task_struct *child;
        child = thread_child(cur, TASK_HANGING);
        // find hanging child, release child kernel stack
        if (child) {
            thread_exit(child, false);
            if (status != NULL) {
                *status = child->exit_code;
            }
            return child->pid;
        }
        child = thread_child(cur, TASK_UNKNOWN);
        if (!child) {  // this process don't have any child, it will return from
                       // wait
            return -1;
        }
        // if have children, this process will be blocked
        thread_block(cur, TASK_WAITING);
    }
}

void release_process_memeory(task_struct *cur) {
    for (int i = pde_index(kernel_all_memory); i < PDE_CNT; i++) {
        pde_t *pde = (pde_t *)cur->page_dir;
        pde += i;
        if (pde->present) {
            for (int j = 0; j < PTE_CNT; j++) {
                pte_t *pte = (pte_t *)pe_phy_addr(pde->phy_pt_addr);
                pte += j;
                if (pte->present) {
                    // release a page
                    uint32_t vaddr = vaddr_by_index(i, j);
                    unmalloc_thread_mem(vaddr, 1);
                }
            }
            // release pte table
            unmalloc_thread_mem(pe_phy_addr(pde->phy_pt_addr), 1);
        }
    }
    // release virtual memory alloc bitmap
    uint32_t bitmap_pg_cnt =
        up2(cur->vir_addr_alloc.bitmap.bytes_len, MEM_PAGE_SIZE) /
        MEM_PAGE_SIZE;
    unmalloc_thread_mem((uint32_t)cur->vir_addr_alloc.bitmap.bits,
                        bitmap_pg_cnt);
}

bool rechild2init(list_node *node, void *arg) {
    task_struct *task = tag2entry(task_struct, all_tag, node);
    uint32_t *parent_pid = (uint32_t *)arg;
    if (task->parent_pid == *parent_pid) {
        task->parent_pid = INIT_PID;
    }
    return true;
}

void process_exit(int status) {
    task_struct *cur = cur_thread();
    if (cur == NULL) {
        return;
    }
    log_info("process_exit:%s\n", cur->name);
    cur->exit_code = status;
    if (!has_other_thread(cur)) {
        // release all user_mode memeory
        release_process_memeory(cur);
    }
    // put all child process to init
    foreach (&all_tasks, rechild2init, &cur->pid)
        ;
    // wake up parent
    task_struct *parent = pid2task(cur->parent_pid);
    if (parent->status == TASK_WAITING) {
        set_thread_status(parent, TASK_READY, false);
    }
    if (cur->vfork_done) {
        complete(cur->vfork_done);
    }
    // hang current process
    set_thread_status(cur, TASK_HANGING, true);
}

void *process_mmap(void *addr, size_t length, int prot, int flags, int fd,
                   int offset) {
    uint32_t ret_vaddr;
    int page_count = up2(length, MEM_PAGE_SIZE) / MEM_PAGE_SIZE;
    if (addr > 0) {
        ret_vaddr = malloc_thread_mem_vaddr((uint32_t)addr, page_count);
    } else {
        ret_vaddr = malloc_thread_mem(PF_USER, page_count);
    }
    if (ret_vaddr == -1) {
        return NULL;
    }
    return (void *)ret_vaddr;
}

int process_sysinfo(uint32_t pid, sys_info *info, int flags) {
    task_struct *task = pid2task(pid);
    if (!task) {
        return -1;
    }
    if (flags & SYS_INFO_MEM) {
        extern phy_addr_alloc_t kernel_phy_addr_alloc;
        extern vir_addr_alloc_t kernel_vir_addr_alloc;
        extern phy_addr_alloc_t user_phy_addr_alloc;
        uint32_t kernel_page_count = count_mem_used(&kernel_vir_addr_alloc);
        uint32_t user_page_count = count_mem_used(&task->vir_addr_alloc);
        uint32_t kernel_phy_page_count = count_mem_used(&kernel_phy_addr_alloc);
        uint32_t user_phy_page_count = count_mem_used(&user_phy_addr_alloc);
        info->mem_info.kernel_mem_used = kernel_page_count * MEM_PAGE_SIZE;
        info->mem_info.user_mem_used = user_page_count * MEM_PAGE_SIZE;
        info->mem_info.kernel_phy_mem_used =
            kernel_phy_page_count * MEM_PAGE_SIZE;
        info->mem_info.user_phy_mem_used = user_phy_page_count * MEM_PAGE_SIZE;
    }
    info->mem_info.mem_page_size = MEM_PAGE_SIZE;
    memcpy(info->pwd, task->pwd, sizeof(task->pwd));
    if (flags & SYS_INFO_CPU_INFO) {
        info->cpu_info.rdtscp = get_rdtsc();
    }
    return 0;
}

bool ps_info_visitor(list_node *node, void *arg) {
    void **args = (void **)arg;
    ps_info *ps = (ps_info *)args[0];
    size_t *count = (size_t *)args[1];
    size_t *index = (size_t *)args[2];
    if (*index >= *count) {
        return false;
    }
    task_struct *task = tag2entry(task_struct, all_tag, node);
    memcpy(&(ps[*index].name), task->name, sizeof(task->name));
    ps[*index].pid = task->pid;
    ps[*index].ppid = task->parent_pid;
    ps[*index].status = task->status;
    *index = *index + 1;
}
int process_ps(ps_info *ps, size_t count) {
    size_t index = 0;
    void *args[3] = {(void *)ps, (void *)&count, (void *)&index};
    foreach (&all_tasks, ps_info_visitor, args)
        ;
    return index;
}

void clone_thread_exit(int (*func)(void *), void *func_arg) {
    int exit_code = func(func_arg);
    exit(exit_code);
}
typedef struct {
    int (*func)(void *);
    void *func_arg;
    void *child_stack;
    int flags;
} clone_process_args;
void start_clone_process(void *arg) {
    task_struct *thread = cur_thread();
    clone_process_args clone_args;
    memcpy(&clone_args, arg, sizeof(clone_process_args));
    unmalloc_thread_mem((uint32_t)arg, 1);
    if (clone_args.child_stack == NULL) {
        // alloc a page for user mode stack, and assign esp to to page end to be
        // stack top
        uint32_t vaddr = user_mode_stack_addr();
        vaddr = malloc_thread_mem_vaddr(vaddr, USER_STACK_SIZE / MEM_PAGE_SIZE);
        void *user_stack = (void *)(vaddr + MEM_PAGE_SIZE);
        clone_args.child_stack = user_stack;
    }
    clone_args.child_stack =
        stack_push(clone_args.child_stack, clone_args.func_arg);
    clone_args.child_stack =
        stack_push(clone_args.child_stack, clone_args.func);
    clone_args.child_stack =
        stack_push(clone_args.child_stack, NULL);  // not need return address
    process_kernel2user(thread, clone_thread_exit, clone_args.child_stack);
}

uint32_t process_clone(int (*func)(void *), void *child_stack, int flags,
                       void *func_arg) {
    task_struct *cur = cur_thread();
    if (!cur) {
        return -1;
    }
    task_struct *thread = (task_struct *)alloc_kernel_mem(1);
    if (!thread) {
        return -1;
    }
    init_thread(thread, cur->name, cur->priority);
    if (flags & CLONE_PARENT) {
        thread->parent_pid = cur->parent_pid;
    } else {
        thread->parent_pid = cur->pid;
    }
    thread->pid = alloc_pid();
    uint32_t args_addr = malloc_thread_mem(
        PF_KERNEL,
        1);  // for clone params, will be released after enter into user mode
    clone_process_args *args = (clone_process_args *)args_addr;
    memset(args, 0, sizeof(clone_process_args));
    args->func = func;
    args->func_arg = func_arg;
    args->child_stack = child_stack;
    args->flags = flags;
    if (flags & CLONE_VM) {
        // all threads share virtual memory in a process
        // so, copy page_dir and vir_addr_alloc
        thread->page_dir = cur->page_dir;
        memcpy(&thread->vir_addr_alloc, &cur->vir_addr_alloc,
               sizeof(vir_addr_alloc_t));
        if (args->child_stack == NULL) {
            args->child_stack = (void *)malloc_thread_mem(PF_USER, 1);
        }
    } else {
        init_process_mem(thread);
    }

    sprintf(thread->name, "clone_from_%d", cur->pid);
    thread_create(thread, start_clone_process, (void *)args);

    pushr(&ready_tasks, &thread->general_tag);
    pushr(&all_tasks, &thread->all_tag);
    if (flags & CLONE_VFORK) {
        DECLARE_COMPLETION(vfork_done);
        thread->vfork_done = &vfork_done;
        wait_for_completion(&vfork_done);
    }
    return thread->pid;
}
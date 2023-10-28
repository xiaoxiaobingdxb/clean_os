#include "process.h"

#include "../memory/config.h"
#include "../memory/mem.h"
#include "common/asm/tool.h"
#include "common/cpu/contrl.h"
#include "common/lib/string.h"
#include "common/tool/math.h"
#include "thread.h"

uint32_t user_mode_stack_addr() {
    extern uint64_t kernel_all_memory, user_all_memory;
    uint32_t vaddr = (uint32_t)(kernel_all_memory + user_all_memory -
                                MEM_PAGE_SIZE);  // last 4K virtual memory
    return vaddr;
}

extern void intr_exit();

#define EFLAGS_IOPL_0 (0 << 12)  // IOPL0
#define EFLAGS_MBS (1 << 1)
#define EFLAGS_IF_1 (1 << 9)
void start_process(void *p_func) {
    task_struct *thread = cur_thread();
    thread->self_kstack += sizeof(thread_stack);
    process_stack *p_stack = (process_stack *)thread->self_kstack;
    p_stack->edi = p_stack->esi = p_stack->ebp = p_stack->esp_dummy = 0;
    p_stack->ebx = p_stack->edx = p_stack->ecx = p_stack->eax = 0;
    p_stack->gs = 0;
    p_stack->fs = p_stack->es = p_stack->ds = USER_SELECTOR_DS;

    p_stack->err_code = 0;
    p_stack->eip = p_func;
    p_stack->cs = USER_SELECTOR_CS;
    p_stack->eflags = (EFLAGS_IOPL_0 | EFLAGS_MBS | EFLAGS_IF_1);

    // alloc a page for user mode stack, and assign esp to to page end to be
    // stack top
    uint32_t vaddr = user_mode_stack_addr();
    vaddr = malloc_thread_mem_vaddr(vaddr, USER_STACK_SIZE / MEM_PAGE_SIZE);
    p_stack->esp = (void *)(vaddr + MEM_PAGE_SIZE - 1);
    p_stack->ss = USER_SELECTOR_SS;
    asm("mov %[v], %%esp\n\t"
        "jmp intr_exit" ::[v] "g"(p_stack)
        : "memory");
}

// alloc page_dir for process and init virtual memory bitmap
void init_process_mem(task_struct *thread) {
    thread->page_dir = create_uvm();
    init_user_vir_addr_alloc(&thread->vir_addr_alloc);
}

void process_execute(void *p_func, const char *name) {
    task_struct *thread = (task_struct *)alloc_kernel_mem(1);
    init_thread(thread, name, TASK_DEFAULT_PRIORITY);
    thread->pid = alloc_pid();
    thread_create(thread, start_process, p_func);
    init_process_mem(thread);
    pushr(&ready_tasks, &thread->general_tag);
    pushr(&all_tasks, &thread->all_tag);
}

uint32_t process_get_pid() {
    task_struct *cur = cur_thread();
    if (cur == NULL) {
        return -1;
    }
    return cur->pid;
}

int copy_process(task_struct *dest, task_struct *src) {
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

    for (uint32_t idx = 0; idx < src->vir_addr_alloc.bitmap.bytes_len * 8;
         idx++) {
        if (bitmap_scan_test(&src->vir_addr_alloc.bitmap,
                             idx)) {  // had been used
        }
    }

    unalloc_kernel_mem((uint32_t)buff, 1);
}

uint32_t process_fork() {
    task_struct *cur = cur_thread();
    if (!cur) {
        return -1;
    }
    task_struct *thread = (task_struct *)alloc_kernel_mem(1);
    init_thread(thread, cur->name, cur->priority);
    int err = copy_process(thread, cur);
    if (err) {
        return -1;
    }
    thread->parent_pid = cur->pid;
    thread->pid = alloc_pid();
    const char *name = "fork";
    memcpy(thread->name, name, strlen(name));
    pushr(&ready_tasks, &thread->general_tag);
    pushr(&all_tasks, &thread->all_tag);
    return thread->pid;
}

uint32_t process_wait(int *status) {}

void process_exit(int status) {}

int process_execve(const char *filename, char *const argv[],
                   char *const envp[]) {
    return 0;
}

void *process_mmap(void *addr, size_t length, int prot, int flags, int fd,
                   int offset) {
    uint32_t ret_vaddr;
    int page_count = up2(length, MEM_PAGE_SIZE) / MEM_PAGE_SIZE;
    if (addr > 0) {
        ret_vaddr = malloc_thread_mem_vaddr((uint32_t)addr, page_count);
    } else {
        ret_vaddr = malloc_thread_mem(page_count);
    }
    return (void *)ret_vaddr;
}
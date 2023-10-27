# 内核态与用户态

## 内核态与用户态的分割原因
每个进程都被分成了内核态和用户态，在前面的内存分页管理中，操作系统将物理内存和虚拟内存都分为了内核内存和用户内存两大块，这是为了方便内存的管理。而在任务调度中说到每个进程都占有完事的n个G的虚拟用户内存和共享1G的物理/虚拟内核内存。对于每个进程自己的虚拟用户内存，自己想怎么折腾都行，但对于共享的内核内存，则不允许随便操作，因为内核内存的虚拟内存与物理内存是一一映射的，所有的进程信息都保存在内核内存中，如果能随便操作，就容易把其他进程信息搞坏。同样的，用户进程不应该影响到内核的功能，例如不能随便修改其他进程的调度信息，否则就会导致内核的调度算法失效。跟共享内核内存一样，进程也共享了所有的硬件设备，例如cpu、键盘、鼠标、硬盘、屏幕等，为了让进程在访问这些外围设备时能够整齐有序，也不能让进程随便直接使用这些设备。

为此，cpu设计了权限系统。在低级权限状态下不允许访问高权限的数据，也不允许执行高权限的代码，同时也不允许使用高权限的cpu指令。

## cpu privilege ring(0-3)
对于权限的定义，cpu将权限类比成了4个环，分别是ring0、ring1、ring2、ring3，环的数字越大表示权限越小。在每个环内都只允许执行一定的cpu指令，高权限能够访问执行低权限的内存和代码，能够使用低权限的指令，但反之则不行。
![cpu privilege ring](../img/cpu_privilege_ring.png)

对于操作系统的内核态和用户态则可以对应于cpu权限的环定义，内核态在ring0，用户态在ring3。这样就可以实现内核态可以访问所有内存使用所有指令，能够实现操作系统的所有功能，而在用户态则只能访问部分内存并且使用部分指令。

权限用为数据和访问，代码的执行以及cpu指令的使用。对于数据和代码，其实都是内存，所以其实就是给内存指定权限级别。cpu指令的使用则需要在执行指令代码时，则cpu去检查。

## 权限的定义
gdt用于定义段，gdt的第一项都是8字节，其中40-47共8位表示段的属性，这8位的5-6两位表示段的权限DPL(Descriptor Privilege Level)，其值就是前面说到了cpu的环，0-3分别表示三个权限的环，数字越小权限越大。
![gdt](../img/segment_desc.png)
![gdt_access](../img/gdt_access.png)

用0表示内核态的段，3表示用户态的段，这样分别有了内核态代码和数据和用户态的代码与数据，解决了内存(代码与数据)的权限定义。

有了定义，当访问数据段和代码段时，还得知道当前的权限是什么。在开启gdt分段管理后，段寄存器中保存的是段选择子，段选择子的高位(>1)保存的是当前段在gdt数组中的index，而低2位就是用什么权限访问段，称为CPL(Current Privilege Level)，只有当CPL<=DPL时才能访问段选择子指定的段，否则就会触发段保护异常。其中的CS保存的代码段选择子，不仅代表着访问代码的权限，还表示可以执行cpu的指令权限级别。如果CPL(CS)>0，则cpu指令的使用也会受限。

在段间跳转或跨段访问数据时，需要指令要跳转到或访问的段选择子，此时指定的段选择子的低2位同样是表示权限，但此时不叫CPL(因为此时的CPL还在CS寄存器中保存着)，而是RPL(Request Privilege Level)，同样当RPL<=DPL时才能进行段间跳转或跨段数据访问。

总结下来基于gdt的权限管理围绕着三个变量进行，并且需要满足条件：`CPL<=DPL && RPL<=DPL`

对于中断中的idt，每一项也是8个字节，同样也有40-47共一个字节表示access，在access中的5-6位表示权限DPL。与段的访问一样，在使用`int`指令触发中断时，也需要保证满足条件`CPL<=DPL && RPL<=DPL`

## 进程的内核态与用户态
在[任务与调度](./task_schedule.md)一文中实现了任务，可以将任务理解成线程，也可以理解成进程，当两个task使用相同的虚拟内存映射表时，就可以实现共享虚拟内存，但线程之间是不能共享stack的，因此寄存器以及esp还是需要保存在task中，而在task中保存相同的page_dir以此实现共享虚拟内存。

进程任务的创建其实就是在内核内存中创建一个pcb(task_struct)，并且将其添加到任务调度的队列中等待时钟中断到来上cpu执行。既然是在内核内存中创建，那么当然只能是在内核态才能进行此操作。同样，由于时钟中断实现的调度函数需要访问牌内核内存中的pcb，这个时钟中断的调度函数也应该是在内核态执行的，所以进程任务首次被调度时是在内核态。

## 由内核态进入用户态
由于内核态和用户态的CPL不同，也就是要修改段寄存器中的值。

前面提到的中断，在触发中断时，会保存当前所有的段寄存器信息以及eip、esp到tss中，然后获取tss中定义的段寄存器信息和esp0恢复到对应的寄存器并执行中断处理程序，中断处理程序执行完后用iret指令会从tss中取出之前触发中断时保存的所有段寄存器信息以及eip、esp等。

在让进程从内核态进入用户态，其实就是像中断iret时用tss中的信息恢复到对应的寄存器中。由于进程首次被调度时是时钟中断触发的，所以刚好可以伪造时钟中断的iret，替换要iret的对应寄存器的值。进程非首次调度时，也是由时钟中断触发的，此时是要调度到另一个进程，switch_to到next的进程时，next任务前一次被调度时刚好是在schedule里，因此当next任务重新被调度回来时，eip刚好也指向了schedule，此时正常回到schedule。此时esp已经转到了next进程的内核栈，再回到中断处理程序的iret，由iret回到next进程的用户态。

为了在进程任务被首次调度时能够iret到用户态，就不能让调度switch_to回到schedule而是去执行kernel_thread函数，在kernel_thread函数中再伪造iret，用此iret进入到用户态。之前thread_start函数开启一个任务指定的func参数是任务的入口函数，如果是个进程，则需要将此func参数指定成能够伪造iret的函数，这里实现start_process函数。
```
typedef struct {
    uint32_t intr_no;
    // all register for popal
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp_dummy;	// not use, is declared to hold 4byte in stack for popal
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;

    // below field is declared to simulate interrupt iret, start process from kernel mode into user mode
    uint32_t err_code; // not use, is delcared to hold 4byte in stack for iret
    void (*eip) (void); // iret to the code address, really is process entry function
    uint32_t cs; // process cs selector
    uint32_t eflags;
    void* esp; // the really stack in user mode
    uint32_t ss; // process ss selector
} process_stack;

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
    extern uint64_t kernel_all_memory, user_all_memory;
    uint32_t vaddr = (uint32_t)(kernel_all_memory + user_all_memory -
                                MEM_PAGE_SIZE);  // last 4K virtual memory
    vaddr = malloc_thread_mem_vaddr(vaddr, 1);
    p_stack->esp = (void *)(vaddr + MEM_PAGE_SIZE - 1);
    p_stack->ss = USER_SELECTOR_SS;
    asm("mov %[v], %%esp\n\t"
        "jmp intr_exit" ::[v] "g"(p_stack)
        : "memory");
}
```

上面的start_process中使用了process_struct，这个结构与task_struct非常相似(几乎就是一模一样)。
## 


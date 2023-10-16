# 任务与调度
在多任务系统中，多个任务可以并发地运行在同一个cpu核心上，这里的并发并不是同时，真正的同时运行其实叫作并行。并发的实现一般依赖于多任务系统的时间片调度，是将一个完整的任务分割成n个时间片，每个时间片只能在cpu上运行一小段时间，这一小段时间片运行后需要让出cpu资源(包含内存资源/内存页表)，让下一个任务占有cpu运行其时间片。

## 任务调度核心机制
### 调度的形式——与函数调用的异同
任务的调度其实跟函数的调用非常相似，都是暂停当前的执行流转而去执行另一个设定好的执行流，可以让下面的代码演示：
```c
task_struct *test1, *test2;
void test_thread_func1(void *arg) {
    char* name = (char*)arg;
    for (; ;) {
        siwtch_to(cur, test_tread_func2);
    }
}
void test_thread_func2(void *arg) {
    char* name = (char*)arg;
    for (; ;) {
        switch_to(cur, test_thread_func1);
    }
}
void main() {
    test1 = thread_start("thread_test1", test_thread_func1, "thread_test1");
    test2 = thread_start("thread_test2", test_thread_func1, "thread_test2");
}
```

在test_thread_func1和test_thread_func2中的函数体中，都是在一个for循环中去执行一个函数switch_to，表明当前函数暂停转去执行另一个函数，最终执行流会变成test_thread_fucn1->est_thread_fucn2->est_thread_fucn1->est_thread_fucn2->......
这样这两个函数就可以交替运行，如果这两个函数交替运行切换的很快，我们就可以把他们当成是在并行地运行。

虽然任务调度跟函数调度是如此的相似，都是转去执行另一个任务后再回来继续执行当前任务， 但switch_to并不能像普通的函数调用那样，直接去调用另一个函数，如果直接调用函数，就会从被调用函数的第一行代码重新开始执行；而任务的调度是需要去执行下一个任务在前一次被调度执行的下一行代码。还是上面的代码，当test_thread_func1执行到for循环中调用switch_to调度到test_thread_func2，在test_thread_func2中也执行到for循环再次调度回test_thread_func1时，这时的test_thread_func1是需要继续向下执行而不是又从头开始执行。因此可以发现任务调度的核心是实现类似于函数调用的一种机制，但这种机制需要让被调度的任务能够继续执行上一次被调度的代码。

### 任务的数据保存与恢复
还是在上一节的代码，虽然这两个任务的实现代码几乎一模一样，但可以发现其传的参数实际上是不一样的。它们的参数按照函数调用的规则，都是储存在stack上的，但cpu的esp只有一个，因此需要在任务被调度后能够用stack取到当前任务stack上的数据，就需要更新esp的值；既然要更新，当然也就需要在任务被换下cpu之前保存esp的值，以便在下次被换上cpu的时候用于更新到esp中。

除了esp寄存器的值用于保存和恢复stack的数据，各种段寄存器的值也一样，每个任务的数据可能存在不同一段中，因此这些段寄存器中的值也需要在任务调度过程中在换下cpu的时候保存，在换上cpu的时候恢复。

在上一篇文章[内存分页管理](./memory_paging.md)中介绍到内存分页管理的核心是内存映射表，也即使pte和pde数组表示的table；在任务调度的过程中，由于每个任务可能都有自己的分页表(不是可能，而是一样)，而page_dir最终也是保存在cr3寄存器中的，因此跟esp和各种段寄存器一样，也需要保存和恢复cr3寄存器的值。对于分页表的保存与恢复，其实是为了实现内存分页的根本目的，也就是在同一个物理地址空间中让每个任务都有自己的虚拟地址空间。每个任务可以将同一个虚拟地址映射到不同的物理地址，这样就可以让每个任务“误以为”自己可以拥有所有的内存。为了实现这个效果，其实就是利用内存分页管理的虚拟内存->物理内存的映射表，map中的key是虚拟内存，而value是物理内存，不同的任务有不同的map，因此可以实现key相同而value不同，所以每个任务都需要保存自己的map。

此处多说一嘴：如果两个任务用同一个map，也就能用同一个虚拟地址访问到同一个物理地址，在代码层面从表面看来，就是访问访问同一个变量能够得到同一个值，也就是说实际上这两个任务共享的(虚拟)内存,(由于物理内存只有一块，所有的任务当然只能共享物理内存)。两个任务访问同一个变量还能得到不同的值？当然能!在代码中的变量实际上就是一个虚拟内存地址，在函数中的局部变量和函数形参是在栈上分配的内存，变量实际上就是相对于栈基址的一个offset，当然就是一个内存地址，在函数外的全局声明或初始化的一个变量实际上就是数据段的一个tag，其实也是相对于其对应数据段的一个offset，当然也是一个内存地址。由于开启了内存分页机制，所有访问的内存都被当成了虚拟内存，需要经过虚拟内存地址->物理内存地址的映射后才能访问硬件设备的内存，因此在不同的任务中如果分页表不同，同一个虚拟内存地址对应的物理内存地址当然可能不同，也即是同一个变量的值当然可能不同。

在操作系统的概念中，同一个进程共享内存，而一个进程可以拥有多个线程，线程是可以并发执行的。因此可以将任务当成线程，而当n个任务拥有了相同的内存分页表时，表明这些任务可以共享虚拟内存，因此这n个任务属于同一个进程。

### 任务调度的时机和下一个任务的选择
在上面的代码中通过switch_to模拟性地描述了任务调度，但真实的任务不可能要求程序员在代码中去写这种调度代码，调度应该是对程序员(任务代码)透明：程序员不应该知道什么时候调度，也不应该知道调度到哪个任务。由于引出两个问题：任务调度的时机和下一个任务的选择。

由于不应该在代码中指定调度的时机，因此需要有一个机制在任务的函数在执行时，能够间接地帮任务函数执行switch_to，假设我们有一个这样的函数schedule，在这个函数中只做一件事，就是得到当前的任务和下一个要被调度的任务，执行switch_to。但由于cpu同时只能有一个执行流，在执行任务的函数时，不可能主动地去调用schedule函数(编译期插入代码不算)，否则就又绕回去了。

cpu除了根据eip一行一行地执行代码指令外，还有另外一种机制：中断，用于实现在特定事件发生时，暂停当前执行流转去执行另一个预定程序。并且这个切换的触发还不影响cpu当前正在运行的执行流，因此可以通过中断来提供任务调度的时机。而刚好硬件层面就预定了这样一种中断——时钟中断，让我们可以设定时钟执行n次时触发一次中断，暂停cpu当前的执行流，转而去执行我们预定的中断程序。如果我们在中断程序中去调用schedule函数，就可以调用switch_to函数，保存当前任务的eip、esp、各种段寄存器的值，用被调度上cpu的任务的值更新到对应寄存器中去，当中断程序执行完后，cpu恢复执行流，但这时发现各种寄存器中的值已经被更新了，就会去函数更新后的代码，访问的数据也是更新过后的，以此就实现了任务的切换。因此时钟中断可以作为任务调度时机的一种非常好的实现方式。

对于下一个任务的选择，这其实涉及到任务调度算法。当然最简单的就是让任务一个一个的执行，也就是FIFO算法，可以让每个任务都平等地得到cpu。但这种方式会让需要快速执行的任务不能更多地得到运行的机会，而让不那么需要及时运行的任务又得到了多余的运行机会，因此，FIFO并不是在所有情况下都适用。因此下一个任务的选择实际上就是一个算法题，需要在不同的场景，或者有不同需求时，如何设计一套规则，让不同的任务有其适合的运行机会。

## 任务的上下文切换

在汇编程序中，程序运行到哪里其实是eip寄存器控制的，eip中总是保存着当前运行的指令的代码段地址，一些跳转指令比如jmp和call/ret其实最终都是通过修改eip中的值来实现运行代码的跳转，因此为了让当前任务停下来让cpu转去执行下一个任务，就需要像函数调用一样，保存当前任务的eip，然后将eip的值更新为下一个任务要执行的代码指令地址。

除此之外，由于函数调用使用stack作为参数传递的基础，当然包括call/ret也是将参数的返回地址也储存在stack中，因此可以想象得到，stack也是任务的核心数据，每个任务都有自己的栈，任务调度后需要将esp也更新。
### 核心数据结构——thread_stack
我们定义如下的数据结构，用于在任务调度时保存和恢复对应寄存器的值。
```c
typedef struct {
    uint32_t ebp, ebx, edi, esi;
    void (*eip)(thread_func *func, void *func_arg);
    void(*unused_retaddr);
    thread_func *func;
    void *func_arg;
} thread_stack;
```
上面的代码第一行是定义了4个寄存器，这些寄存器之所以在任务调度时要被保存和恢复，主要是以下原因：
1. ebp主要用于栈的基址变址寻址，在一个任务函数中，可能ebp要在多个指令行使用
2. ebx也是主要用于基址变址寻址
3. edi和esi用于做批量操作，这个批量操作过程中如果发生了任务调度，如果edi和esi变化了，会导致数据批量错误，因此也需要保存和恢复
当然，这些也都是cpu的建议需要保存和恢复的寄存器。

第二行是一个函数类型的指针，也就是在任务调度后要执行的函数，可以理解成在中断程序执行结束后，要将此值更新到eip中，让cpu执行流继续去执行这个函数。这个函数的参数是另一个函数和一个void指针类型的参数，表示的是要在当前要被执行的函数中去调用这个参数中的函数，且这个函数的实参要传这个void指针类型的参数。这里的这个thread_func实际上就是真正需要执行的函数，在任务第一次被调度时，会执行这个eip对应的函数，但在任务被换下cpu时，这个变量会储存任务函数中下一行代码的地址，因此这个变量有两个用途，分别是首次被调用时指向任务的函数，被调度换下cpu时，保存下一行代码的地址。

第三行的unused_retaddr变量正如其名，没有使用的返回地址，这是因为在函数调用时，会把的返回地址也push到stack中，这个变量就用来占用保存函数的返回地址，之所以不用这个变量，是因为在执行完switch_to后，并不会回到调用switch_to的调用处，而是去执行eip变量中的代码地址。

第四行和第五行是用于首次运行时去执行eip中保存的入口函数所需要的参数。

thread_stack中保存了任务上下文切换的核心数据，但任务还会有一些另外的数据，比如任务名、任务状态、任务优先级等，为了更有层次感，定义另一个结构体task_struct，并用这个task_struct包含thread_stack，就用32位的指针变量self_kstack保存thread_stack的内存地址。
```c
typedef struct {
    uint32_t *self_kstack;
    char name[TASK_NAME_LEN];
    task_status status;
    list_node general_tag, all_tag;
    uint32_t priority;
    uint32_t ticks; // time running at cpu
    uint32_t elapset_ticks; // time running at cpu all life
} task_struct;
```

### 核心实现——switch_to
switch_to先用c函数声明成函数并声明成extern
extern void switch_to(task_struct *cur, task_struct *next);
再用汇编实现:
```c
	.text
    .global switch_to
switch_to:
    // the next code address in stack top

    // store current task registers for thread_struct
    push %esi
    push %edi
    push %ebx
    push %ebp

    // store current task esp into first param
    // because the pcb store in current task stack
    mov 0x14(%esp), %eax // first param is cur task_struct pointer
    mov %esp, 0x0(%eax) // update current task thread_struct, thread_struct at the first and it's offset is 0

    // restore next task register value into registers
    mov 0x18(%esp), %eax // second param is next task_struct pointer
    mov 0x0(%eax), %esp //  restore next task pcb into stack

    // restore register value
    pop %ebp
    pop %ebx
    pop %edi
    pop %esi
    // here, the stack top is eip of next task
    // with c language function invoke rule, after switch_to return, next code address in eip, also is next task next code at last running
    ret
```
switch_to的代码不多，但每一行都跟thread_stack结构体的字段定义的顺序和长度息息相关。

首先是按照thread_stack结构体的定义先push ebp/ebx/edi/esi这4个寄存器的值到stack中，注意到push的顺序跟字段定义的顺序是相反的，这是因为stack是向低地址增长的。虽然这里的汇编代码只push了4个寄存器的值，但不要忘了，函数调用call时会将当前函数的下一行代码的指令地址也push到stack，因此其实在第一行push的时候，stack的栈顶已经是调用switch_to函数的下一行代码了，而由于thread_stack中定义的第二行就是eip，因此这时的thread_stack的eip中保存的就是这个下一行代码的地址。当switch_to重新调度回这个任务时，就会从thread_stack中取出这个eip继续执行。

然后是两行指令`mov 0x14(%esp), %eax`和`mov %esp, 0x0(%eax)`，正如代码注释所言，使用基址寻址，取出距离栈顶20个字节的数据，由于刚才4个push再加上call指令导致push的switch_to的返回处的下一行代码地址，也就是5个4字节数据，刚好是20个字节，因此`mov 0x14(%esp), %eax`寻址的就是call之前push的数据，根据函数调用传参规则，call之前push的就是函数的参数，并且push从右向左，这个0x14的offset对应的就是siwthc_to的第一个参数cur表示当前任务结构体task_struct。`mov %esp, 0x0(%eax)`这行代码表示将当前的esp寄存器的值赋值给cur的第一个变量，也就是task_struct中的self_kstack，这个self_kstack虽然只是个32位的指针变量，实际上我们会在这个变量中保存thread_stack。因此stack中的5个4字节数据，就分别对应着switch_to的调用处的下一行代码地址和4个寄存器的值。这时就能对应上thread_stack结构体的定义了。

0x14对应着switch_to的第一个参数cur，那么0x18就对应着switch_to的第二个参数next，这个参数也是task_struct，因此接下来的两行指令`mov 0x18(%esp), %eax`和`mov %0x0(%eax), %esp`就是将switch_to的第二个参数next取出来并将其第一个字段self_kstack的值赋值给esp。前面说到这个self_kstack保存的是thread_stack，而thread_stack就是下一个任务保存各种寄存器和eip值的结构里，因此执行完这两行指令后，stack中从栈顶向下依次就是ebp, ebx, edi, esi。这里为啥就跟thread_stack中字段的定义顺序相同了呢？这是因为next在被调度下cpu时保存这些寄存器值的时候，也就是switch_to的前四行代码，是反着顺序push的，已经按着thread_stack的定义顺序反着了，因此这里就可以直接pop出来了，所以这里接下来的4行pop就对应着最前面的4行push。

最后是一个ret指令，由于先前pop出了4个寄存器的值，此时栈顶中的值就是thread_stack的eip字段的值了，首次被调度上cpu时，eip保存的是kernel_thread函数，因此时会ret到kernel_thread
```c
static void kernel_thread(thread_func *func, void *func_arg) {
    sti();
    func(func_arg);
}
```
unused_retaddr字段就冒充了switch_to调用kernel_thread函数时的返回地址，这时因为kernel_thread函数中会调用真正的任务函数，并且不会返回，直到任务函数执行完毕，并且任务函数执行完毕后也不会通过kernel_thread返回。

而非首次调度上cpu时，其实eip中保存的还是调用switch_to处的下一行代码地址用于返回，之所以switch_to能够正常返回到任务函数中，是因为中断机制，在中断执行前会保存cpu的正常执行流的下一行代码地址，中断处理函数执行完毕后，会返回到正常执行流，类似于函数调用。

## 时钟中断调用switch_to
实现了switch_to用于任务的上下文切换，接下来就是要实现前面说到的任务调度的时机和下一个任务的选择了。
下一个任务的选择使用链表的队列形式来实现，每次都从队首pop下一个任务，并且将当前任务push到队尾。这里的队列用于保存所有可以等待直接上cpu运行的任务，因此叫这个队列为ready_tasks队列。

优先级的实现可以给每个task_struct设置一个ticks，这个ticks的初始值是priority，每次触发时钟中断时，都将这个task_struct的ticks--，当ticks减到0时表示这个任务的时间片用完，要被调度下cpu，转而去pop出下一个任务，将其调度上cpu。
```c
void schedule() {
    task_struct *cur = cur_thread();
    if (cur->status == TASK_RUNNING) {
        cur->elapset_ticks++;
        if (cur->ticks-- > 0) {  // continue run current task
            return;
        }
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
```

## task_struct的内存位置
经过前面switch_to函数的实现分析，可以知道task_struct肯定是保存在stack上的，但由于每个任务都有一个stack，并且这个stack上的数据用于任务调度，属于内核行为，因此尽量将这个stack保存在内核内存中。所以与任务的真实的stack区别开，在创建task时，专门为其在内核中申请4k也就是刚好一页的虚拟内存用于保存task_struct，并将task_struct保存在stack初始化时的栈底，也就是4k的offset=0的位置，当需要获取当前task时，就可以读取esp的值，将其对齐到4k处，就能得到当前task的task_struct
```c
task_struct *cur_thread() {
    uint32_t esp;
    __asm__ __volatile__("mov %%esp, %[v]" : [v] "=r"(esp));
    return (task_struct *)(esp & 0xfffff000);
}
```
## task的初始化
到此为止已经实现了任务的切换，最后只剩下任务的创建初始化了，初始化也比较简单，只是将task_struct和thread_stack对应的字段赋初始值就行，特别需要注意的是thread_stack的eip在初始化的时候需要赋值成kernel_thread函数。
```c
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

task_struct *thread_start(const char *name, uint32_t priority, thread_func func,
                          void *func_arg) {
    task_struct *thread = (task_struct *)alloc_kernel_mem(1);
    init_thread(thread, name, priority);
    thread_create(thread, func, func_arg);
    pushr(&ready_tasks, &thread->general_tag);
    pushr(&all_tasks, &thread->all_tag);
    return thread;
}
```
task的初始化是在内核虚拟空间中先申请1页的内存用于任务的内核栈，初始化task_struct和thread_stack，最后将任务push到ready_tasks的队尾。用户任务通过thread_start进行初始化。我们想把内核的执行流也包装成一个任务，由于内核已经运行起来了，并且已经有栈了，因此不需要再额外申请1页虚拟内存，只需要直接当前的esp。同样由于内核任务已经运行起来了，就不再需要将其push到read_tasks队列中了。
```c
void enter_main_thread() {
    main_thread = cur_thread();
    init_thread(main_thread, "main", 31);
    pushr(&all_tasks, &main_thread->all_tag);
}
```

## 验证任务调度
为了验证任务调度的情况，以下代码在内核本身的基础上又使用thread_start创建了两个新任务
```c
task_struct *test1, *test2;

void test_thread_func1(void *arg) {
    char* name = (char*)arg;
    int count = 0;
    for (; ;) {
        count++;
        hlt();
    }
}
void test_thread_func2(void *arg) {
    char* name = (char*)arg;
    int count = 0;
    for (; ;) {
        count++;
        hlt();
    }
}

void main() {
    // test_uv_page_dir();
    enter_main_thread();
    test2 = thread_start("thread_test2", 10, test_thread_func2, "thread_test2");
    test1 = thread_start("thread_test1", 15, test_thread_func1, "thread_test1");
    for (;;) {
        // int a = 10 / 0;
        hlt();
    }
}
```
在这两个新任务的函数中执行count的累加，这个count由于是局部变量，因此每个任务都有自己的count，并且在任务调度的过程中其值是能够继续累加。在这两个任务函数的for循环里打断点，可以发现它们会被调度，而且由于设置了优先级，test_thread_func1会被test_thread_func2调度的时间更长，这验证了优先级调度产生了效果。

## 任务内存的管理
现在由于还没有给任务设置自己单独的虚拟内存映射表，因此cr3中的值在任务调度的过程中没变，所有任务使用同一个虚拟内存映射表。
前面说到如果任务是个进程，就需要有单独的内存管理，也就是需要为每个进程的任务分配一个虚拟内存映射表。这将会在后续的进程加载执行与堆内存分配的部分实现。
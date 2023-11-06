# 进程的创建与退出
在[内核态与用户态](./kernel_user_mode.md)一文中使用start_process实现了从内核态切换到用户态，这可以在内核的main函数中创建一个新进程。但main函数其实也是运行在内核态的，如果要在用户态创建进程，则需要做一些额外的处理。同时，由于进程占用了以下几种资源，在进程执行完毕后还需要提供操作让进程能够释放这些资源
1. 进程的内核栈(pcb，task_struct)，在内核内存中
2. 进程申请的内存，在内核内存和用户内存都有
3. 进程的虚拟内存映射表，在内核内存中
4. 进程的虚拟内存分配器，在内核内存中
5. 进程的pid，在内核内存中
6. 进程的用户态栈，在用户内存

上述资源分布在内核内存和用户内存中，不过，内存的分配与释放实际上就是操作物理/虚拟内存分配器和对应的虚拟内存映射表，内存分配器和虚拟内存映射表都在内核内存中，因此上述资源的分配与释放操作都需要在内核态中执行，所以如果想在用户态创建进程和释放进程资源，则只能通过系统调用的方式

## 进程的创建
进程的创建在前面的thread中已经实现了，然后通过start_process由内核态切换至用户态，但为了实现在用户态创建进程，则提供fork与clone两个syscall

### fork
fork的函数原型是：`pid_t fork();`

fork主要是为了实现对当前进程的一个copy，并且在fork syscall返回时子进程与当前进程处于相同的处。因此fork这个系统调用其实会返回两次，一次是在原进程中返回，返回的pid_t是子进程的pid；另一次是在子进程中返回，返回的pid_t是0。由于fork的两次返回都在相同的代码处，所以可以通过fork返回值来处理是处于原进程还是子进程中。
```c
int test = 0;
int pid = fork();
if (!pid) {
    // run parent code
    test = 1;
} else {
    // run child code
    test = 2;
}
```
同时，由于fork是对原进程的完整copy，所以就算是执行相同的代码，拥有相同的全局变量与局部变量，这些变量的值也是在不同内存中的，所以虽然值是相同的，但后续在原进程和子进程中对这些变量的修改，将产生不同的影响。比如在上面的代码中原进程和子进程都能访问到test这个局部变量，但在原进程中将test修改为1，在子进程中修改为2。在后续对test的访问中，原进程中访问的结果将会是1而子进程中的访问结果会是2。

fork syscall的结果让代码运行看起来有点怪异，同一段代码中访问同一个变量居然有不同的值？这种怪异的感觉是因为我们将对变量的两次访问当成了一个进程，但fork后却是产生了一个新的进程，因此两次访问其实是在不同的进程，不同的进程中访问一个变量其实是访问不同的变量(虽然变量名一样)。或者可以用另外一种视角来看，fork相当于是将同一个代码执行了两次，一个程序被执行两次，在这两次运行中，得到不同的变量值当然就在情理之中了。

fork由于是对原进程的完整copy，所以实现起来就比较简单了。由上面罗列的进程资源可见，其实也就是在内核态生成一个新的task_struct，并将原进程的task_struct的各个字段的值copy到新的task_struct中，同时也copy pcb。这里有个小小的巧妙的地方，由于syscall将用户态调用处的eip压栈到了进程的内核栈中，而内核栈刚好就在pcb的高地址处，所以copy pcb也就相当于copy了进程的内核栈，eip也被copy到新进程的内核栈了。当fork syscall从中断返回时，也会从内核栈中取出这个eip，这个eip刚好就是原进程调用fork的返回地址，所以就能实现fork的子进程能和原进程一样返回到fork的调用返回地址处。

```c
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
```
上面的函数copy_process_mem用于fork syscall将原进程内存copy到子进程中,`memcpy(dest, src, MEM_PAGE_SIZE);`是copy pcb，默认认为pcb为1页，也即是4K。然后使用`init_process_mem`函数分配一个page_dir和虚拟内存分配器，也即分配一个虚拟内存映射表，这里只分配了pde表。接下来的几行代码是给fork的中断退出设置参数，让子进程被时钟中断调度时，能够通过这个中断进入到用户态。最后就是copy用户内存了，这里采用一页一页的copy，但由于cr3中保存着page_dir，cr3在同一时间只能保存一个进程的page_dir，因此在两个进程间copy时需要一个中转的buff，这个buff是在内核内存中申请的，因为所有的进程能够共享内核内存(因为所有的进程的内核内存使用的pte相同，这在给进程创建page_dir时就完成了)。copy完成的最后还需要将buff的内存释放还给内核内存分配器。由于用户态栈是在用户内存的最后一页，因此对用户内存的copy实际上已经copy了用户态栈了。

### clone
前面的fork会copy当前进程的所有内存，但有时可能需要创建一个子进程，子进程能够跟父进程共享内存(相当于创建了线程)，有时可能还想创建一个当前进程的兄弟进程，让新创建的进程的父进程为当前进程的父进程。相对于fork，clone函数会以一个新的函数入口作为新进程的执行起始地址，并且还要支持传入参数，根据参数来判断是否需要额外的内存copy。有时原进程还需要向子进程传入一些参数，所以clone函数需要提供一个参数，用于原进程向子进程传递参数。clone的函数原型如下：
```
#define CLONE_VM (1 << 0) // set if vm shared between process
#define CLONE_PARENT (1 << 1) // set if want the same parent, also create a new process as the cloner' brother
#define CLONE_VFORK (1 << 2) // set if block current process until child process exit
pid_t clone(int (*func)(void*), void* child_stack, int flags, void *func_arg);
```
clone函数的第一个参数是新进程的入口函数，入口函数用一个void*作为参数，返回一个int作为返回码。

第二个参数是当需要和当前进程共享内存时，为新进程配置的用户栈的低地址，默认栈是一个页的大小，在clone的实现中会将用户态栈指向页的高地址，也就是`esp=child_stack+MEM_PAGE_SIZE`

第三个参数是个标志位:
1. CLONE_VM表示新进程与原进程共享内存，由于共享了内存，因此用户内存的最后一页已经被原进程当成用户态栈使用了，所以需要clone通过child_stack传入，如果没有传入，则在原进程中申请一块1页大小的内存当作用户态栈。
2. CLONE_PARENT会创建一个新进程，新进程作为原进程的兄弟进程，也就是新进程的父进程pid与原进程的父进程pid相同
3. CLONE_VFORK的操作跟fork差不多，但fork会有两次返回，父进程和子进程均会返回一次，但vfork会让子进程阻塞父进程，当子进程调用exit退出后父进程才能退出阻塞状态

最后一个参数是func_arg表示第一个参数func的void*类型的参数，这个参数最终会在func被调度执行时传入。一般来说这个都会传一个指针，但这里需要注意的是，如果子进程与原进程没有共享内存的话，子进程中的func函数是无法通过这个指针来访问到参数的内存的。

#### CLONE_VM和CLONE_PARENT的实现
clone的实现与fork类似，也是需要先创建pcb，但由于不需要像fork一样让子进程与父进程一起返回，所以也就不需要进行pcb的copy。如果发现flag中有CLONE_VM参数，则表示要与父进程共享内存(相当于创建了一个线程)，共享内存很容易，跟之前线程的原理一样，让子进程与父进程共享page_dir并且copy一下虚拟内存分配器即可。
```c
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
            args->child_stack =(void*) malloc_thread_mem(PF_USER, 1);
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
```
注意到这里给进程的入口地址是start_clone_process而不是start_process了，这是因为在使用process_execute创建进程时进程在用户态的入口函数不需要参数，而咱们的clone出的新进程的用户态入口函数是需要传个参数func_arg的。还有，clone的flag参数如果有CLONE_VM则需要传入一个child_stack，而不再是直接使用用户内存的最后一页，为此，start_clone_process函数跟start_process的函数原型虽然参数都是个void*类型的，但start_process的参数是进程的用户态入口函数地址，而start_clone_process的却是一个结构体的地址，里面需要包含用户态入口函数地址，入口函数的参数还有用户态的栈地址。因此需要再声明一个结构体：
```c
typedef struct {
    int (*func)(void *);
    void *func_arg;
    void *child_stack;
    int flags;
} clone_process_args;
```
需要注意的是，如果flag中没有CLONE_VM则新进程与原进程无法共享内存，process_clone函数是运行在原进程的内核态的，但start_clone_process已经运行在新进程的内核态，如果这个clone_process_args在原进程的内存中则在新进程的start_clone_process中无法访问。为了解决这个问题，只能再借为内核内存来做个中转，当clone_process_args放入内核内存中，使用完毕后释放掉这块中断的内存。
```c
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

void start_process(void *p_func) {
    task_struct *thread = cur_thread();
    // alloc a page for user mode stack, and assign esp to to page end to be
    // stack top
    uint32_t vaddr = user_mode_stack_addr();
    vaddr = malloc_thread_mem_vaddr(vaddr, USER_STACK_SIZE / MEM_PAGE_SIZE);
    void *user_stack = (void *)(vaddr + MEM_PAGE_SIZE);
    process_kernel2user(thread, p_func, user_stack);
}
```
start_clone_process的实现就跟之前的start_process非常非常相似了，为此，可以将start_clone_process与start_process的共用代码再写到一个新函数process_kernel2user中。这个process_kernel2user函数就只做process_struct的赋值以及通过中断从内核态切到用户态。

在start_clone_process中，由于需要将用户态进程的入口函数参数传入，则需要再次利用函数调用参数的压栈知识了。由于进程的用户态入口函数已经是运行在用户态了，则当然使用的栈也是用户态栈了，函数的参数也肯定是储存在用户态栈上了。为此，可以模拟一下调用前的压栈过程，将入口函数的参数先压入用户态栈中。

为了方便clone出的进程在return后通过自动地调用exit退出进程，这里专门做了个中转设计，让新进程的入口函数变成clone_thread_exit，而将新进程真正的入口函数变成clone_thread_exit的参数传给clone_thread_exit，同时由于clone_thread_exit已经是新进程的入口函数了，则这个函数应该是没有返回地址的，所以也模拟地在用户态栈中最后再压个NULL当成返回地址(为了栈平衡)。根据函数调用参数的压栈顺序是从右向左压的，因此先压真正的入口函数的参数func_arg，再压真正的入口函数的地址，最后模拟地压入一个NULL充当clone_thread_exit的返回地址。在clone_thread_exit中就比较简单地调用一下新进程真正的入口函数，传入其参数，在func返回时，调用exit，自动进行函数的退出。当然，如果func中也有了exit导致进程退出，由于进程会在exit调用时退出调度，以后再也不会被调度上cpu了，所以在clone_thread_exit中的exit也就没有机会执行了。
```c
void clone_thread_exit(int (*func)(void *), void *func_arg) {
    int exit_code = func(func_arg);
    exit(exit_code);
}
```
还需要注意到，clone_thread_exit其实是运行在用户态的(因为它是新进程的入口函数)，所以得使用exit这个syscall，而不是直接调用内核态的exit。先就这样写着，其实clone_thread_exit函数本来应该也需要在用户态内存中的，后续再实现时再将这个clone_thread_exit再copy到用户态内存去。

#### CLONE_VFORK的实现
vfork其实是不太好实现的，vfork需要阻塞当前进程(好办，将当前进程的status设置成waiting即可，从ready_tasks中移除)，那么当子进程执行exit后，还需要唤醒原进程，将原进程的status重新设置为ready并让其重新进入到ready_tasks中，也就是说新进程中需要保存原进程的一个引用。如果新进程刚好是原进程的子进程还好说，则新进程的parent_pid就是原进程的pid，可以通过pid来索引。但如果使用了CLONE_PARENT则新进程是原进程的兄弟进程，就无法通过parent_pid来索引了。

为了解决这个问题，需要在task_struct新增一个字段，在这个字段中保存原进程的索引。咱们也别真正保存原进程的task_struct或者pid了，而是更加抽象一层，在task_struct中用一个字段来表示所有等待此进程(或线程)执行exit而被阻塞的进程，因此这个字段就应该是一个list了。
```c
typedef struct completion_t {
    unsigned int done;
    list wait;
} completion;

typedef {
    xxx
    completion *vfork_done;
    yyy
} task_struct;
```
completion中的wait表示所有等待当前进程的其他进程的list，done字段表示已经完成了，可以让wait的进程位退出了。done是个数字，表示可以退出阻塞的次数，每完成一次只能让wait中的一个进程退出阻塞状态。

为了让completion实现阻塞与退出的功能，为其提供wait_for_completion和complete两个函数，分别来于阻塞进程和退出进程的阻塞状态。在process_clone中判断flag有CLONE_VFORK时，给新进程的vfork_done赋值，并且原进程调用`wait_for_completion(&vfork_done)`来阻塞，实现vfork的能力。后面在进程的exit实现中，取出新进程的vfork_done参数，调用`complte`函数来通知原进程退出阻塞状态。


## 进程的销毁与资源释放
clone函数可以创建新进程，新进程申请了内核栈、用户态内存等一系统操作系统资源，在进程退出销毁时需要将这些操作系统资源释放掉。

对于不同类型的资源，释放策略也不同。如果当前进程是个线程(和原进程共享了内存)，则线程退出时还得检查是否还有其他活着的进程(线程)占用着这些内存。

前面说到这些资源的申请需要在内核态，同样这些资源的释放也同样需要在内核态，所以exit也同样是个syscall。值得注意的是，exit在内核态时用的栈在pcb中，所以exit期间是不能释放pcb的，其他资源倒是可以直接释放掉了。那么pcb该怎么释放呢？这里有两个方案：1是提供一个专门的进程init，这个进程专门用来释放exit了的进程的pcb；2是让父进程能够帮忙释放子进程的pcb。

对于方案1，咱们总不能让init进一刻不停地避免所有进程，判断进程状态，释放所有exit的进程吧，这样太浪费cpu了。因此需要提供一个wait函数，让init进程用wait函数阻塞自己，只有当有进程调用exit时，才让init进程从wait函数中返回并释放exit的进程的pcb。

对于方案2：为父进程提供一个wait函数，父进程可以随时调用wait函数阻塞自己等待子进程exit后自动从阻塞状态返回。

实际上这种方案跟方案1是一样的，方案1相当于把所有的进程的父进程都设置成init进程。当然，对于方案2其实会更优一点，毕竟为了性能init进行希望不要那么经常被调度，而让父进程能够释放子进程的资源无疑是最好的。但如果父进程只clone了子进程而没有执行wait呢？这样还是得init来回收进程了。因此需要用方案2+方案1的结合，用方案2父进程主动释放子进程的pcb，如果父进程没有wait而是自己exit了，则将此父进程的所有子进程交给init进程，相当于将这些子进程以init进程为父进程，当init执行到wait时，回收这些子进程的pcb。

因此进程资源的释放需要用到exit和wait这两个syscall。对于exit，需要在里面释放用户内存等，等到exit触发到wait时，再释放pcb。exit syscall对应着内核态的process_exit函数
```c
void process_exit(int status) {
    task_struct *cur = cur_thread();
    if (cur == NULL) {
        return;
    }
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
```
`has_other_thread`判断当前进程使用了page_dir有没有被其他进程使用，其实就是当前进程有没有跟其他进程共享内存，如果没有共享，相当于page_dir只有当前进程使用了page_dir，exit时就可以释放掉所有的用户态内存和虚拟内存映射表了。然后是用个foreach遍历当前进程的所有子进程，将这些子进程认init进程为父，其实就是将这些子进程的parent_pid设置为init进程的pid(0)。最后是找到当前进程的父进程，如果父进程是waiting状态，则将其设置为ready状态，让其从wait中返回。还有前面说到了vfork，因此还得调用complete函数，让被vfork阻塞的进程也从阻塞状态返回。最后设置当前进程的状态为hanging，等待父进程的wait函数回收当前进程的pcb。

```c
uint32_t process_wait(int *status) {
    task_struct *cur = cur_thread();
    if (!cur) {
        return -1;
    }
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
```
wait syscall对应着内核态的process_wait函数，这是在父进程的内核态执行的。process_wait的主体是一个for循环，在循环里检查处于hanging状态的子进程，如果没找到，就进入到到waiting状态，阻塞自己。如果找到了处于hanging的子进程，则调用thread_exit释放掉子进程的pcb并从wait返回。

为了实现方案1，还需要有一个init进程，在init进程不断的调用wait去回收子进程的pcb，这些子进程是其他进程忘记调用wait而无法回收pcb的子进程。
```c
void init_process() {
    // test_fork();
    sys_info info;
    pid_t pid = get_pid();
    sysinfo(pid, &info);
    test_clone();
    for (;;) {
        int status;
        uint32_t child_pid = wait(&status);
        sysinfo(pid, &info);
        uint32_t cpid = child_pid;
        if (child_pid == -1) {
            yield();
        }
    }
}
```

## 提供一些查询进程信息的syscall
有了前面的fork/clone用于创建进程，exit+wait用户释放进程资源，进程的管理基本上已经齐活了，为了更好的监控进程状态，可以提供一些额外的syscall。

### sysinfo
为了能够方便地提供内存使用情况，提供sysinfo来判断系统相关信息，比如已经使用的内核内存，已经使用的用户物理内存和用户虚拟内存。
```c
int process_sysinfo(uint32_t pid, sys_info *info) {
    task_struct *task = pid2task(pid);
    if (!task) {
        return -1;
    }
    extern phy_addr_alloc_t kernel_phy_addr_alloc;
    extern vir_addr_alloc_t kernel_vir_addr_alloc;
    extern phy_addr_alloc_t user_phy_addr_alloc;
    uint32_t kernel_page_count = count_mem_used(&kernel_vir_addr_alloc);
    uint32_t user_page_count = count_mem_used(&task->vir_addr_alloc);
    uint32_t kernel_phy_page_count = count_mem_used(&kernel_phy_addr_alloc);
    uint32_t user_phy_page_count = count_mem_used(&user_phy_addr_alloc);
    info->kernel_mem_used = kernel_page_count * MEM_PAGE_SIZE;
    info->user_mem_used = user_page_count * MEM_PAGE_SIZE;
    info->kernel_phy_mem_used = kernel_phy_page_count * MEM_PAGE_SIZE;
    info->user_phy_mem_used = user_phy_page_count * MEM_PAGE_SIZE;
    return 0;
}
```
这些内存信息的查询其实也比较容易，内存使用信息全部都在对应的内存分配器中。每个内存分配器中已经使用的内存其实就是被置为1的bit。

### ps
为得到所有的进程信息，提供ps syscall，对应着内核态的process_ps，实现起来也比较简单，for循环遍历all_tasks，取出每个进程的信息即可。
```c
bool ps_info_visitor(list_node *node, void *arg) {
    void** args = (void**)arg;
    ps_info *ps = (ps_info*)args[0];
    size_t *count = (size_t*)args[1];
    size_t *index = (size_t*)args[2];
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
    void* args[3] = {(void*)ps, (void*)&count, (void*)&index};
    foreach(&all_tasks, ps_info_visitor, args);
    return index;
}
```
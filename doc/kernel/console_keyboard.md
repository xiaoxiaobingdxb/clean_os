# 控制台与键盘
在计算机的远古时期，只有键盘作为输入，屏幕作为输出，键盘和控制台的屏幕都被当作设备进行控制。而在Linux中，所有的设备都被虚拟化成了文件，键盘和屏幕则被虚拟化成了标准输入和输出文件，也就是stdin和stdout。对于c语言编程时，使用printf则是输出到stdout，使用gets则是从stdin获取到输入。在计算机的早期，将键盘的输入和屏幕的输出又当成了tty这种设备，因此tty就变成了一种具有输入输出的设备。

## console
在实模式下，可以通过bios的0x10号中断的0xe号子程序实现屏幕的打印，但进入到保护模式后，就不能再使用bios提供的中断了。为了实现屏幕打印，就只能通过显存映射到的虚拟内存进行控制。

在80x25模式下，屏幕可以被理解成每行80共25行的网络，每个网络中可显示一个字符。在彩色模式下的显存，每个字符用两个字节表示，低8位表示字符，高8位表示颜色，而其中颜色又被分别前景色和背景色，前景色表示字体颜色，而背景色则表示整个格子的颜色。颜色的格式是rgb，共3个字节，这样一个字节肯定无法表示一个前景色+背景色，因此文本模式的显存并不支持所有rgb颜色，而是将基础的颜色进行标号，通过标号来表示一种颜色，在一个字节中就可以用前4位表示前景色，后3位表示背景色，可见[Print to Scene](https://wiki.osdev.org/Printing_To_Screen)，颜色与标号的对应关系如下：
| Color Number | Color Name   | RGB         |
| ----         | ----         | ----        |
| 0            | Black        | 0,0,0       |
| 1            | Blue         | 0,0,170     |
| 2            | Green        | 0,170,0     |
| 3            | Cyan         | 0,170,170   |
| 4            | Red          | 170,0,0     |
| 5            | Purple       | 170,0,170   |
| 6            | Brown        | 170,85,0    |
| 7            | Gray         | 170,170,170 |
| 8            | Dark Gray    | 85,85,85    |
| 9            | Light Blue   | 85,85,255   |
| 10           | Light Green  | 85,255,85   |
| 11           | Light Cyan   | 85,255,255  |
| 12           | Light Red    | 255,85,85   |
| 13           | Light Purple | 255,85,255  |
| 14           | Yellow       | 255,255,85  |
| 15           | White        | 255,255,255 |

对于这16种颜色刚好可以用4位表示，在C语言中，可以定义成enum
```c
typedef enum {
    Black = 0,
    Blue,
    Green,
    Cyan,
    Red,
    Purple,
    Brown,
    Gray,
    Dark_Gray,
    Light_Blue,
    Light_Green,
    Light_Cyan,
    Light_Red,
    Light_Purple,
    Yellow,
    White
} console_color;
```

由于每个字符需要两个字节来表示，因此可以定义一个struct来表示屏幕上的一个字符
```c
typedef union {
    struct {
        char ch;
        uint8_t fg : 4;
        uint8_t bg : 3;
    };
    uint16_t v;

} display_char_t;
```

显存的起始地址是0xB8000，共32KB，而每个字符需要2个字节来表示，因此一共就是16K个字符，由于屏幕是80x25的，因此这32KB的显存一共可以有32K/2/(80*25)=8个scene，每个scene的起始地址为0xB8000+idx*(80*25*2),将每个scene定义成一个console_t结构
```c
typedef struct {
    int display_cols, display_rows;  // console width and height
    int cursor_col, cursor_row;      // cursor currently position
    console_color display_fg, display_bg;  // console default color
    display_char_t *display_base;
} console_t;
```
1. display_cols和display_rows分别表示屏幕的宽度和调度，也就是80和25
2. cursor_col和cursor_row表示光标的位置，如果将屏幕左上角视为坐标0点，则cursor_col和cursor_row就是光标的x和y坐标值
3. display_fg和display_bg分别表示显示字符的默认前景和背景色
4. display_base表示每个scene的起始地址

因此，对于要显示一个字符，就是要将光标位置对应的内存地址写入两个字节的数据，并将光标向右移动一位
```c
void putchar_color(console_t *console, char ch, console_color fg,
                   console_color bg) {
    int offset =
        console->cursor_col + console->cursor_row * console->display_cols;
    display_char_t *display = console->display_base + offset;
    display->ch = ch;
    display->fg = fg;
    display->bg = bg;
    cursor_forward(console, 1);
}
```

## keyboard
对于键盘，其作为一种输入设备，是由中断提供事件告知cpu有数据输入的，输入的数据被缓存在中断控制芯片(8259_PIC)中，使用inb指令读取真正的输入数据。键盘的数据输入中断号为0x21。因此在中断向量表中为0x21注册键盘输入处理程序:
```c
bool inited = false;
void init_keyboard() {
    if (!inited) {
        register_intr_handler(INTR_NO_KYB, keyboard_handler);
        init_segment(&lock, 0);
        init_queue(&kbd_buf, wait_fun, wakeup_fun);
        inited = true;
    }
}
```
键盘的数据port为0x60，因此读取键盘输入数据的指令是`inb 0x60, %[v]`

由于键盘每个按键都存在着长按的情况，比如长按shift键的过程中敲击了其他键，就会有另外的字符，比如敲击字母键则表示输入大写字母，如果是敲击'/'键则表示输入'?'。因此键盘的控制芯片将按键的按下和松开分别触发一个中断，分别用一个字节来表示按键的按下与松开。每个按键都用一个字节来表示，则按下的第7位为0，松开的第7位为1。

为了实现ctrl、shift、alt键的长按效果，可以在全局变量中用3个bool变量分别表示ctrl、shift、alt键是否被按下，当判断输入字符为对应键时，判断第7位是0表示被按下了，这个bool变量设置为true，第7位是1时表示松开再将这个bool变量设置为false。当其他按键输入时，判断这三个bool变量的值，以此来实现长按的效果。
上面注册的keyboard_handler的实现如下：
```c
void keyboard_hander(uint32_t intr_no) {
    io_wait();
    uint16_t scan_code = inb(KBD_DATA_PORT);
    if (scan_code == 0xe0) {
        ext_scancode = true;
        return;
    }
    if (ext_scancode) {
        scan_code = 0xe000 | scan_code;
        ext_scancode = false;
    }
    bool break_code = (scan_code & 0x0080) != 0;
    if (break_code) {
        uint16_t make_code = (scan_code &= 0xff7f);
        switch (make_code) {
        case ctrl_l_make:
        case ctrl_r_make:
            ctrl_down = false;
            break;
        case shift_l_make:
        case shift_r_make:
            shift_down = false;
            break;
        case alt_l_make:
        case alt_r_make:
            alt_down = false;
            break;
        default:
            break;
        }
        return;
    }
    if (scan_code > 0x0 && scan_code < 0x3b || scan_code == alt_r_make ||
        scan_code == ctrl_r_make) {
        bool shift = false;
        if (shift_down) {
            shift = true;
        }
        uint8_t idx = (scan_code &= 0x00ff);
        char ch = keymap[idx][shift];
        if (ch) {
            _log_debug("keyboard input ch=%c\n", ch) int idx = queue_len(&kbd_buf);
            buf[idx] = ch;
            queue_put(&kbd_buf, (void*)buf + idx);
            return;
        }
        switch (scan_code) {
        case ctrl_l_make:
        case ctrl_r_make:
            ctrl_down = true;
            break;
        case shift_l_make:
        case shift_r_make:
            shift_down = true;
            break;
        case alt_l_make:
        case alt_r_make:
            alt_down = true;
            break;
        case caps_lock_make:
            caps_lock_down = !caps_lock_down;
            break;
        default:
            break;
        }
        return;
    }
}
```

## 基于阻塞队列的键盘数据缓冲

由于键盘的输入数据在键盘输入中断处理程序中进行读取，读取后总要在一个地方暂存起来，当有其他进程要读取键盘输入数据时，就可以从这个地方直接读取，这个暂存键盘输入数据的地方叫作键盘缓冲。

当有进程要读键盘缓冲时，如果缓存中没有数据则将进程进入block状态进行等待，当进行键盘中断处理程序读取到键盘输入时，将等待键盘缓冲的进程唤醒，从缓冲中取出数据给进程。因此这个缓冲其实是一个基于进程阻塞调度的队列。

### 缓冲队列的数据结构

在前面进程调度中，ready_tasks也是一个队列，每次从队头取出一个task上cpu运行，运行消耗完时间片后将其重新插入到队尾巴。这样的队列是用链表实现的，采用链表的原因是，要经常从队头取和从队尾插入数据，如果用数组实现的话，每次取和插入都需要操作整个数组，这样的时间复杂度太大了，因此采用了链表这种以空间换时间的数据结构。链表由于每个节点除了要保存数据本身，还要额外的前置指针pre和后置指针next，相当于每个节点都需要额外的2个指针类型长度(32位cpu上占4个字节)。

在键盘输入缓冲中，每个数据都是一个ascii字符，每个字符占用一个字节，如果也用链表来实现，则缓冲的每个节点都需要1byte+4byte+4byte=9byte，真实的数据只有1个字节却要用9字节来储存，这样太浪费内存了，因此链表不是一个好的选择。除了链表那就只有数组了，但前面也说过，数组实现的队列在入队和出队时由于要移到整个数组，其时间复杂度很高，所以也不能直接使用数组。如果数据的储存能直接使用数组而数据的入队和出队能将操作链表一样，那就完美了。

数组实现队列时，在入队时，其实就是在队尾处插入数据，也就是`array[len++] = value`时间复杂度是O(1)，时间复杂度高的其实是出队操作，出队时需要将数组的每一位都依次向左移动一位。要是出队能像链表的删除首个节点，将head指向第2个节点就能实现入队的O(1)时间复杂度。直接使用数组的出队需要将数组的每一位都左移一位的原因是需要用下标0表示第1个节点，而如果使用一个整数变量tail来指示第一个节点在数组中的位置，是不是就可以不再必须用下标0来表示第1个节点，也就不需要在出队时将数组中的每一位都左移一位了，这个时候的出队操作就变成了`value=array[tail++]`，而此时的len也就不能表示队列的长度，当然，还需要指示着队尾节点在数组中的下标，用整数变量head来表示，队列的真正长度其实就是`head-tail`。

这样的队列还有一个致命的缺点，那就是队列的实际使用长度还是数组的长度，因为当head移动到数组的最后一位时，就不能再进行入队操作了。但这时候可能之前已经有出队操作导致`tail>0`了，实际上数组的前面部分还有空余位置可以用来储存数据。为了解决队尾超过数组长度的问题，可以将数组的头和尾连接起来变成一个环，当head到达数组尾部时，将其移动到数组的头部，也就变成了`head=(head++)%arr_len;array[head]=value`。当然，这种环状的数组实现的队列还是有问题的，那就是入队的追尾，也就是当`head+1=tail`的时候，入队的数据会覆盖队头的数据。为了解决这个问题，得在入队之前先判断一下队列是否已经满了，判断队满的条件是`head+1==tail`，而出队前，也要判断一下队空，队空的条件是`head==tail`。

### 阻塞队列的实现
阻塞队列的缓冲要实现的是：
1. 在队空时，阻塞读取数据的task；
2. 而在队满时要阻塞写入数据的task;
3. 真正写入数据时，如果有处于阻塞的读取task，就需要将其唤醒
4. 真正读取时，如果有处于阻塞的写入task，就需要将其唤醒
   
在读取数据阻塞或写入数据阻塞时，当前cpu还在当前task中，因此实现阻塞实现起来还是比较简单的，只需要将当前task的状态设置为TASK_BLOCKED，再触发一次schedule就能实现。而要唤醒，则需要在当前task中去将另一个task状态设置为TASK_READY，要实现此效果，就需要在队列中保存被阻塞task的标识。为此，使用producer表示被阻塞的写入数据task，用consumer表示被阻塞的读取数据task。
```c
#define queue_buf_size 1024
typedef void(*block_fun)(int*);
typedef struct {
    int producer, consumer;
    block_fun wait, wakeup;
    void* buf[queue_buf_size];
    int head, tail;
} block_queue;
```
对于wait函数的实现，正如前面所说，只需要将当前task设置为TASK_BLOCKED即可：
```c
void segment_wait(segment_t *seg, pid_t *pid) {
    eflags_t state = enter_intr_protect();
    task_struct *cur = cur_thread();
    ASSERT(cur != NULL);
    if (cur->pid == 0) {
        if (cur == NULL) {
            ASSERT(false);
        }
    }
    *pid = cur->pid;
    if (seg->count > 0) {
        seg->count--;
    } else {
        pushr(&seg->waiters, &cur->waiter_tag);
        set_thread_status(cur, TASK_BLOCKED, true);
    }
    leave_intr_protect(state);
}
```
上面的segment_wait函数，将当前被阻塞的task的pid使用引用类型进行输出，在block_queue中保存为producer或consumer。

对于wakeup函数的实现，将producer或consumer作为pid的task的状态设置为TASK_READY。
```c
bool find_by_pid_visitor(list_node *node, void *arg) {
    task_struct *task = tag2entry(task_struct, waiter_tag, node);
    void **args = (void **)arg;
    pid_t *pid_ptr = (pid_t *)args[0];
    if (task != NULL && task->pid == *pid_ptr) {
        args[1] = (void *)node;
        return false;
    }
    return true;
}
void segment_wakeup(segment_t *seg, pid_t *pid) {
    eflags_t state = enter_intr_protect();
    list_node *tag;
    if (*pid != 0) {
        void *args[2] = {(void *)pid, NULL};
        foreach (&seg->waiters, find_by_pid_visitor, (void *)args)
            ;
        tag = args[1];
    } else {
        tag = popl(&seg->waiters);
    }
    if (tag != NULL) {
        task_struct *task = tag2entry(task_struct, waiter_tag, tag);
        ASSERT(task != NULL);
        *pid = task->pid;
        thread_ready(task, false, true); // dispatch firstly
        // set_thread_status(task, TASK_READY, false);
    } else {
        seg->count++;
    }
    leave_intr_protect(state);
}
```
有了segment_wait和segment_wakeup来实现block_queue_t的wait和wakeup函数，block_queue_t的阻塞队列就非常好实现了。
```c
void init_queue(block_queue *queue, block_fun wait_fun, block_fun wakeup_fun) {
    queue->wait = wait_fun;
    queue->wakeup = wakeup_fun;
    queue->consumer = queue->producer = 0;
    queue->head = queue->tail = 0;
}
int next_pos(int pos) {
    return (pos + 1) % queue_buf_size;
}

bool queue_full(block_queue* queue) {
    return next_pos(queue->head) == queue->tail;
}
bool queue_empty(block_queue *queue) {
    return queue->head == queue->tail;
}
size_t queue_len(block_queue* queue) {
    if (queue->head >= queue->tail) {
        return queue->head - queue->tail;
    }
    return queue_buf_size - (queue->tail - queue->head);
}
void* queue_get(block_queue* queue) {
    while (queue_empty(queue) && queue->wait != NULL)
    {
        queue->wait(&queue->consumer);
    }
    if (queue_empty(queue)) {
        return NULL;
    }
    void *value = queue->buf[queue->tail];
    queue->tail = next_pos(queue->tail);
    if (queue->producer != 0 && queue->wakeup != NULL) {
        queue->wakeup(&queue->producer);
        queue->producer = 0;
    }
    return value;
}
void queue_put(block_queue* queue, void *vlaue) {
    while (queue_full(queue) && queue->wait != NULL)
    {
        queue->wait(&queue->producer);
    }
    queue->buf[queue->head] = vlaue;
    queue->head = next_pos(queue->head);
    if (queue->consumer != 0 && queue->wakeup != NULL) {
        queue->wakeup(&queue->consumer);
        queue->consumer = 0;
    }
}
```

## 虚拟文件系统实现tty设备
前面用console实现了屏幕的显示，用键盘中断处理程序实现了键盘数据的读取，而tty设备就是将二者结合起来，将console作为tty的输出，将keyboard作为tty的输出。前面也说到，Linux将tty也作为虚拟文件，文件的读取就是tty的输入，文件的写入就是tty的输出。

### 虚拟文件系统定义
由于tty要作为虚拟文件，当然还会有其他类型的文件，比如磁盘上真正的文件，所以虚拟文件系统还要管理着不同类型的文件，这些不同类型的文件都来自一种硬件设备，真正的文件肯定是来自于磁盘，而tty的文件就来自于tty这种虚拟的硬件设备，真正的硬件设备就是屏幕和键盘。为此，需要先定义虚拟文件的设备类型
```c
#define FS_MOUNTP_SIZE 512
typedef enum {
    FS_DEV,
} fs_type_t;
struct _fs_ops_t;
typedef struct _fs_desc_t {
    char mount_point[FS_MOUNTP_SIZE];
    fs_type_t type;
    list_node node;  // organize by link_list
    struct _fs_ops_t *ops;
} fs_desc_t;
typedef struct _fs_ops_t {
    int (*mount)(fs_desc_t *fs, int major, int minor);
    void (*unmount)(fs_desc_t *fs);
    int (*open)(fs_desc_t *fs, const char *path, file_t *file);
    ssize_t (*read)(file_t *file, byte_t *buf, size_t size);
    ssize_t (*write)(file_t *file, const byte_t *buf, size_t size);
    void (*close)(file_t *file);
    int (*seek)(file_t *file, size_t offset);
    int (*ioctl)(file_t *file, int cmd, int arg0, int arg1);
} fs_ops_t;
```
在内核进行初始化时，就需要初始化虚拟文件系统支持的硬件设备类型，因此声明一个设备类型的列表。
```c
#define FS_TABLE_SIZE 10
fs_desc_t fs_table[FS_TABLE_SIZE];
fs_ops_t* fs_ops_table[FS_TABLE_SIZE];
list free_fs, mounted_fs;
void init_fs_table() {
    init_list(&free_fs);
    for (int i = 0; i < FS_TABLE_SIZE; i++) {
        pushr(&free_fs, &fs_table[i].node);
    }
    init_list(&mounted_fs);
    extern fs_ops_t devfs_ops;
    memset(&fs_ops_table, 0, sizeof(fs_ops_table));
    fs_ops_table[FS_DEV] = &devfs_ops;
}
```

为了让内核的虚拟文件系统支持一种设备类型的文件，需要将其进行注册，这个注册在虚拟文件系统里叫作mount，也就是挂载。比如电脑上插入了一块新硬盘，要让操作系统识别到这个新硬盘，就需要先将这个硬盘挂载到一个目录。
```c
fs_desc_t *mount(fs_type_t type, const char *mount_point, int dev_major,
                 int dev_minor) {
    void *args[2] = {(void *)mount_point, NULL};
    foreach (&mounted_fs, mounted_fs_visitor_by_mount_point, (void *)args)
        ;
    fs_desc_t *fs;
    if (!args[1]) {  // not found mounted fs, to create a new fs
        list_node *fs_node = popl(&free_fs);
        if (!fs_node) {
            goto mount_fail;
        }
        fs = tag2entry(fs_desc_t, node, fs_node);
        memcpy(fs->mount_point, mount_point, FS_MOUNTP_SIZE);
        fs_ops_t *ops = fs_ops_table[type];
        if (!ops) {
            goto mount_fail;
        }
        fs->ops = ops;
        if (fs->ops->mount(fs, dev_major, dev_minor)) {
            goto mount_fail;
        }
        pushr(&mounted_fs, &fs->node);
    } else {
        // had been mounted, fail
        goto mount_fail;
    }
    return fs;

mount_fail:
    if (fs) {
        pushr(&free_fs, &fs->node);
    }
    return NULL;
}
```
为了让内核支持tty设备，在初始化时就将tty设备进行挂载，并将tty设备挂载到`/tty`目录下
```c
void init_fs() {
    init_fs_table();
    mount(FS_DEV, "/dev", DEV_TTY, 0);  // mount default /dev
}
```
对于FS_DEV这种虚拟文件设备类型，用devfs实现其fs_ops_t接口，在对应的函数中调用device设备io操作。tty也是一种device设备，要读写device设备，得提供devcie_id，device_id保存在file中。

### tty文件的open
无论是tty设备文件还是普通的文件，要读写都需要先open，普通文件的open是找到文件路径对应的磁盘地址，而设备文件的open则是获取到device_id，而不同进程可能需要同时对同一个文件进行读写，因此文件的描述需要保存在进程中，这就是所谓的文件描述符。文件描述符起到的作用是在一个进程中标识一个文件，不同的进程中相同的文件描述符可能会指向不同的文件，因此可以简单地将文件描述符理解成进程中文件列表的下标，打开一个文件就是将一个文件加入到进程的文件列表中。

系统调用open对应着内核的sys_open，输入一个文件路径，加一些控制用的flag，返回一个文件描述符，后续读写文件就靠这个文件描述符。
下面的sys_open根据文件路径的前缀得到文件的设备类型，获取到已经挂载设备的fs_desc_t，真正打开文件的操作就是要调用fs_desc_t中对应设备的open函数。
```c
/**
 * @brief /a/b/c -> b/c
*/
const char* path_next_child(const char *path) {
    if (*path == '/') {
        path++;
    }
    while (*path++ != '/');
    return (const char*)path;
}

fd_t sys_open(const char *name, int flag, ...) {
    void *args[2] = {(void *)name, NULL};
    foreach (&mounted_fs, mounted_fs_visitor_by_mount_point, (void *)args)
        ;
    if (!args[1]) {
        goto open_fail;
    }
    fs_desc_t *fs = (fs_desc_t *)args[1];
    file_t *file = alloc_file();
    if (!file) {
        goto open_fail;
    }
    fd_t fd = task_alloc_fd(file);
    if (fd < 0) {
        goto open_fail;
    }
    file->mode = flag;
    file->desc = fs;
    name = path_next_child(name);
    memcpy(file->name, name, 0);
    fs->ops->open(fs, name, file);
    return fd;
open_fail:
    if (!file) {
        free_file(file);
    }
    return FILENO_UNKNOWN;
}
```
为了调用tty控制读取键盘和写数据到屏幕上显示 ，tty设备属于devfs文件系统，所以tty对应的文件路径为`/dev/tty0`，这里的0表示tty的第1个设备，也就是第一个console(显存最多支持8个)。

前面已经将devfs这种device设备挂载到'/dev'目录，所以用`/dev/tty0`得到了前面已经挂载的devfs_opts，在devfs的实现中，open又是调用了device_open，由于二级目录是tty，因此找到了tty这种类型的device_type，这里的device_type对应着device的major字段。
```c
int devfs_open(fs_desc_t *fs, const char *path, file_t *file) {
    for (int i = 0; i < sizeof(devfs_type_table) / sizeof(devfs_type_t); i++) {
        devfs_type_t type = devfs_type_table[i];
        size_t len = strlen(type.name);
        if (memcmp(type.name, path, len)) {
            continue;
        }
        int minor = 0;
        if (strlen(path) <= len || str2num(path + len, &minor)) {
            minor = 0;
        }
        dev_id_t dev_id = device_open(type.device_type, minor, &file->mode);
        if (dev_id < 0) {
            return -1;
        }
        file->dev_id = dev_id;
        file->pos = 0;
        file->type = type.file_type;
        return 0;
    }
    return -1;
}
```
进到device_open中，tty这种device已经在初始化时注册到device_desc_t列表中了，用传入的tty的major找到tty的device_desc_t，再用同样的方式调用tty的device_desc_t的open函数，也就是tty_open。最后返回的device_id就是打开的device在devices中的下标，后续要读写device时也通过这个下标找到对应的device。
```c
extern device_desc_t dev_tty_desc;
device_desc_t *device_desc_table[] = {&dev_tty_desc};
#define DEVICES_SIZE 128
device_t devices[DEVICES_SIZE];

dev_id_t device_open(int major, int minor, void *arg) {
    device_t *free_dev = NULL;
    for (int i = 0; i < DEVICES_SIZE; i++) {
        device_t *dev = devices + i;
        if (dev->open_count == 0) {
            free_dev = dev;
        } else if (dev->desc->major == major && dev->minor == minor) {
            dev->open_count++;
            return i;
        }
    }
    device_desc_t *desc = NULL;
    int desc_size = sizeof(device_desc_table) / sizeof(device_desc_t*);
    for (int i = 0; i < desc_size; i++) {
        if (device_desc_table[i]->major == major) {
            desc = device_desc_table[i];
            break;
        }
    }
    if (desc && free_dev) {
        free_dev->desc = desc;
        free_dev->minor = minor;
        free_dev->arg = arg;
        if (free_dev->desc->open(free_dev) == 0) {
            free_dev->open_count = 1;
            return free_dev - devices;
        }
    }
    return -1;
}
```

tty对应的device的minor表示tty的下标，每个tty都有一个console。在open_tty函数中，对键盘和屏幕控制台进行初始化即可。
```c
int tty_open(device_t *dev) {
    if (dev->minor < 0 || dev->minor >= TTY_SIZE) {
        return -1;
    }
    tty_t *tty = ttys + dev->minor;
    tty->console_idx = dev->minor;
    int flags = 0;
    if (dev->arg != NULL) {
        flags = *((int *)dev->arg);
    } else {
        flags = TTY_IN_N_RN | TTY_OUT_ECHO | TTY_OUT_R_N;
    }
    tty->in_flags = flags;
    tty->out_flags = flags;
    init_console(dev->minor);
    init_keyboard();
    return 0;
}
```

### tty文件的read和write
和open一样，read和write就是根据open得到的文件描述符找到对应的tty。跟open的过程几乎完全一样，进行以下步骤：
1. 在虚拟文件系统层：用fd_t，一个整数类型的变量作为下标先去task_struct的file_table中找到对应的file，调用文件设备类型的read或write函数进入到虚拟设备层
2. 在虚拟设备层：用file中的device_id在devices中得到对应的tty device，调用设备的read或write函数进入到tty设备层
3. 在tty设备层：根据device的minor字段找到对应的tty，tty的read操作就是读取键盘缓冲的中数据，使用阻塞式读取函数，tty的write操作就是将数据写入到console。
```c
tty_t *get_tty(device_t *dev) {
    if (dev->minor < 0 || dev->minor >= TTY_SIZE || dev->open_count <= 0) {
        return NULL;
    }
    return ttys + dev->minor;
}
ssize_t tty_write(device_t *dev, uint32_t addr, const byte_t *buf,
                  size_t size) {
    tty_t *tty = get_tty(dev);
    if (!tty) {
        return -1;
    }
    for (int i = 0; i < size; i++) {
        switch (buf[i]) {
        case '\n':
            if (tty->in_flags | TTY_IN_N_RN) {
                console_putchar(tty->console_idx, '\r');
            }
            break;
        default:
            break;
        }
        console_putchar(tty->console_idx, (char)buf[i]);
        console_update_cursor_pos(tty->console_idx);
    }
    return size;
}
ssize_t tty_read(device_t *dev, uint32_t addr, byte_t *buf, size_t size) {
    tty_t *tty = get_tty(dev);
    if (tty == NULL) {
        return -1;
    }
    size_t len = 0;
    while (len < size) {
        char *value = (char *)queue_get(&kbd_buf);
        if (value == NULL) {
            continue;
        }
        if (*value == '\r' && (tty->out_flags & TTY_OUT_R_N)) {
            *value = '\n';
        }
        if (tty->out_flags & TTY_OUT_ECHO) {
            tty_write(dev, 0, value, 1);
        }
        buf[len++] = *((byte_t *)value);
        if ((*value) == '\n' && (tty->out_flags & TTY_OUT_LINE)) {
            break;
        }
    }
    return len;
}
```

## stdin&stdout&stderr
stdin、stdout、stderr都是文件描述符，分别表示标准输入、标准输出和标准异常。标准输入输出和标准异常输出默认指的就是键盘和屏幕控制台，所以stdin、stdout以及stderr都应该指向tty设备文件。在虚拟文件系统中讲到一个文件描述符在一个进程中对应着一个文件，因为文件描述符就是task_struct的file_table数组中文件的下标，那么为啥3个文件描述符能对应着同一个文件呢？当然行，同一个数组中三个元素当然能储存同一个文件指针。

对于stdin、stdout、stderr这三个文件描述符指向同一个文件，需要先使用open系统调用打开一个tty设备得到一个文件描述符，先设置为stdin吧，再把stdout和stderr给dup到得到的这个文件描述符对应的文件上，因此还需要提供dup系统调用。这里dup提供两种系统调用，分别是dup和dup2，对应着内核的sys_dup和sys_dup2：
```c
fd_t sys_dup(fd_t fd);
fd_t sys_dup2(fd_t dst, fd_t source) {
```
dup的输入为一个文件描述符，得到一个输入的文件描述符对应的文件的新的文件描述符，也就是得到一个新的fd，这个新的fd和输入的参数fd指向同一个file，新的fd是自增的。dup2跟dup一样，只不过指定了新的fd的值，相当于将dst指向source对应的file。

为了实现stdin、stdout、stderr三个不同的文件描述符指向同一个文件，需要调用的是dup2，dup2系统调用的内核实现sys_dup2代码如下：
```c
fd_t sys_dup2(fd_t dst, fd_t source) {
    if (dst == source) {
        return dst;
    }
    file_t *file = task_file(source);
    if (!file) {
        return -1;
    }
    file_t *old = task_file(dst);
    if (old) {
        free_file(file);
        task_free_fd(dst);
    }
    if (task_set_file(dst, file, true) == 0) {
        ref_file(file);
        return dst;
    }
}
```
最核心的就是调用task_set_file，其实现在task中，正如前面所说，就是将fd作为下标，将file指针储存到fd下标的数组中。
```c
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
```

tty作为虚拟文件进行读写已经准备就绪，用如下代码进行测试：
```c

void test_kbd() {
    fd_t fd = open("/dev/tty0", 1 << 0 | 1 << 10 | 1 << 11 | 1 << 12);
    dup2(STDIN_FILENO, fd);
    dup2(STDOUT_FILENO, fd);
    dup2(STDERR_FILENO, fd);
    int buf_size = 32;
    char *buf = mmap_anonymous(buf_size);
    ssize_t count = 0;
    size_t get_size = 0;
    while ((count = read(STDIN_FILENO, (void *)buf, buf_size)) > 0) {
        get_size = count;
    }
}
```
这里只读了，是因为open的时候设置了flag中带有1 << 11，在tty定义中表示echo，也就是read的时候也直接进行write，读取的过程中直接将键盘的输入回显到屏幕上(实现shell控制台的效果)

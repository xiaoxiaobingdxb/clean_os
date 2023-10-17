# 中断

## 中断的执行逻辑
中断是由硬件提供的，cpu通过中断号去找预定义的中断处理程序，并且暂停当前执行流，保存现场，执行完中断处理程序后再恢复现场继续执行，非常类似于回调函数。

在实模式中bios提供了各种中断用于提供硬件级别的系统功能，比如屏幕打印、读取硬盘等。但开启保护模式后，cpu的段访问模式发生了改变，中断也变成了像gdt一样，使用idt描述各种中断，称为中断向量表idt。

## 中断向量表

idt也是一个数组，数组的每个元素都是64位，且数组的下标即是中断号，参考osdev中[中断向量表的定义](https://wiki.osdev.org/IDT#Table)，因此可以定义以下结构体:
```c
typedef struct {
    uint16_t entry_low15;
    uint16_t selector;
    uint8_t zero;
    uint8_t attr;
    uint16_t entry_high15_31;
} intr_desc;
```
1. entry_low15和entry_high15_31分别表示中断处理程序的低16位和高16位
2. selector是中断处理程序的段选择子
3. zero是8位的保留位
4. attr是中断向量的属性

atttr字段比较重要的是第0位表示类型
```c
#define GATE_TYPE_TASK 0x5 // 任务门
#define GATE_TYPE_INT_16 0x6 // 16位中断门
#define GATE_TYPE_TRAP_16 0x7 // 16位陷阱门
#define GATE_TYPE_INT_32 0xE // 32位中断门
#define GATE_TYPE_TRAP_32 0xF // 32位陷阱门
```
clean_os现在面向于32位，由于要构造中断向量表，因此第0位选择0xE表示此项为中断门

attr字段的第5位表示此中断的权限，0表示系统级权限，3表示用户级权限，这跟gdt比较相似

attr字段的第7位表示此中断向量的使能位，为1表示启用此中断向量

gdt通过lgdt指令加载，同样的，idt也通过lidt指令加载，且加载的结构体都一模一样。lidt指令也需要一个48位的操作数，低16位表示idt的长度，也就是有多少个中断向量；中间16位表示idt的低16位，高16位表示idt的高16位
```c
static inline void lgdt(uint32_t start, uint32_t size) {
    struct {
        uint16_t limit;
        uint16_t start15_0;
        uint16_t start31_15;
    } gdt;

    gdt.start31_15 = start >> 16;
    gdt.start15_0 = start & 0xffff;
    gdt.limit = size - 1;
    __asm__ __volatile__("lgdt %[g]" ::[g] "m"(gdt));
}
```

## idt的构造
前面说到idt每个中断向量的描述结构体intr_desc，zero字段一般设置成0，select段选择子由于在前面加载gdt时只有一个代码段，因此直接使用这个代码段选择子"8"即可，attr默认使用`((IDT_DESC_P << 7) + (IDT_DESC_DPL0 << 5) + IDT_DESC_32_TYPE)`表示启用这个中断向量，权限为系统级，描述为一个中断向量类型

intr_desc最重要的成员还剩一个中断处理程序了，因此可以定义一个中断处理程序的数组，数组与idt一样，其下标与对应着中断号。中断处理程序跟普通的函数还不一样，在c语言函数的调用规则中，会将参数和函数返回地址都依次压栈，虽然中断处理程序被调用时参数被压栈，但中断处理程序没有返回地址，因此需要构造一个没有ret指令的函数，且这个函数的最后必须是iret指令。所以中断处理函数无法使用正常的c语言实现，而是需要用汇编来实现，但为了后续方便扩展，不可能每添加一个中断处理函数都用汇编实现，因此可以先抽出中断处理函数的通用逻辑，在这个通用逻辑中调用c语言实现的真正的中断处理函数。

为了达到上述的目的，在idt.h头文件中用extern声明中断处理程序数组，表示会在其他地方定义这个数组`extern intr_handler intr_entry_table[INT_DESC_CNT];`，并且在intr.c中定义数组intr_handlers，这个intr_handlers数组就是用c语言实现的真正的中断处理函数，会在汇编实现的通用中断处理函数中调用

接着在intr.S中定义这个intr_entry_table并用.global导出
```c
.data
    .global intr_entry_table
intr_entry_table:
.macro intr_entry intr_no, push_err_code
    .text
    .extern intr_handlers
intr_entry_\intr_no:
    .if \push_err_code > 0 // if not error, push 0 to balance stack
    push $0
    .endif

    // store all registers
    push %ds
    push %es
    push %fs
    push %gs
    pushal

    mov $0x20, %al
    outb %al, $0xa0
    outb %al, $0x20

    push $\intr_no
    mov intr_handlers + 0x4 * \intr_no, %eax
    call  *%eax
    add $0x04, %esp
    popal
    pop %gs
    pop %fs
    pop %es
    pop %ds
    add $0x04, %esp // skip error code
    iret

    .data
    .long intr_entry_\intr_no
.endm
```
上面的代码在数据段定义了intr_entry_table这个tag，在汇编中，不论是函数还是数组或者其他什么字段，都使用tag来定义一个名字。这是因为汇编中其实是没有数据类型的，而只有byte/bit这一种数据类型，数组或函数或字段都可以描述成一个内存地址，而定义tag时其实就是将定义一个内存地址的助记符。

在gnu的汇编语法中，使用
```c
.macro method arg1 arg2 ... argn
xxx
.endm
```
这样形式来定义宏，在宏中使用\argx来引用宏的参数

由于中断处理程序需要感知到中断号，因此可以用宏来定义中断处理程序的通用逻辑部分。intr_entry这个宏有两个参数分别是intr_no表示中断号，push_err_code表示是否需要`push $0`来在stack中占位。这是因为cpu对于不同的中断，可能有些已经push了异常码，有些没有push，相当于函数中的可选参数，但由于需要使用stack平衡，因此使用push_err_code这个参数来判断是否需要push一个0到stack中使stack平衡，方便在最后统一pop。

接下来的中断汇编代码就跟普通的汇编实现函数一样，首先是push所有段寄存器，pushal保存通用寄存器的值，当然最后也一样需要pop对应寄存器用于恢复，并且由于之前`push $0`传入异常码，因此使用`add $0x04, %esp`来pop出这个异常码使栈平衡。中间的两行outb指令是告诉中断芯片已经处理了中断。

然后就是真正的通用函数调用，按照c语言函数调用的规则，函数的参数需要push到stack中，因此先`push $\intr_no`。接着对intr_handlers这个数组进行索引寻址，由于intr_handlers数组中保存的是中断处理程序的地址，因此每一项都是32位(4字节)，因此intr_handlers + 4 * intr_no就是intr_no这个中断号对应的中断处理程序，这个寻址也可以理解成`intr_handlers[intr_no]`。然后就是使用call指令直接调用中断处理程序的函数，最后也需要用`add $0x04, %esp`来将之前push的intr_no给pop出来使栈平衡。最后的最后，需要像函数实现一样，使用中断处理程序的特殊指令iret退出中断处理函数。

前面的这些定义都是为了使用宏定义intr_entry_\intr_no这个中断处理程序，而intr_entry_table是要被定义成一个中断处理程序的数组，数组的每一项都是中断处理程序的地址，因此最后还是要回到数据段来，用.long伪指令写入intr_entry_\intr_no的地址

到此为止还是在宏定义里，最后还得调用宏生成真正的数据和代码，因此在intr.S文件的最后注是调用intr_enty这个宏，传入每个中断号用于生成中断处理程序并给intr_entry_table填充数值
```c
    .text
    #include "intr_no.h"
intr_entry INTR_NO_DIVIDE,1 // divide zero
intr_entry 0x01,1
intr_entry 0x02,1
intr_entry 0x03,1 
intr_entry 0x04,1
intr_entry 0x05,1
intr_entry 0x06,1
intr_entry 0x07,1 
intr_entry 0x08,0
intr_entry 0x09,1
intr_entry 0x0a,0
intr_entry 0x0b,0 
intr_entry 0x0c,1
intr_entry 0x0d,0
intr_entry 0x0e,0
intr_entry 0x0f,1 
intr_entry 0x10,1
intr_entry 0x11,0
intr_entry 0x12,1
intr_entry 0x13,1 
intr_entry 0x14,1
intr_entry 0x15,1
intr_entry 0x16,1
intr_entry 0x17,1 
intr_entry 0x18,0
intr_entry 0x19,1
intr_entry 0x1a,0
intr_entry 0x1b,0 
intr_entry 0x1c,1
intr_entry 0x1d,0
intr_entry 0x1e,0
intr_entry 0x1f,1 
intr_entry INTR_NO_TIMER,1	// timer
intr_entry INTR_NO_KYB,1	// keyboard
intr_entry 0x22,1
intr_entry 0x23,1
intr_entry 0x24,1
intr_entry 0x25,1
intr_entry 0x26,1
intr_entry 0x27,1
intr_entry 0x28,1
intr_entry 0x29,1
intr_entry 0x2a,1
intr_entry 0x2b,1
intr_entry 0x2c,1
intr_entry 0x2d,1
intr_entry 0x2e,1
intr_entry 0x2f,1
```

## 8259_PIC芯片
由于cpu不可能为每个中断都建立一个引脚用于中断信号的输入输出，因此在中断的设计中专门用一个8259_PIC的芯片用于处理中断，所有的中断都先用这个芯片先处理，得到一个中断号，再让这个芯片与cpu连接，将中断信息输入给cpu用于触发中断，执行中断程序函数。因此8259_PIC芯片也需要相应的初始化工作，具体内容可参考osdev的(8259_PIC)[https://wiki.osdev.org/8259_PIC]详解
```c
void pic_init() {
    uint8_t a1, a2;

    a1 = inb(PIC1_DATA);  // save masks
    a2 = inb(PIC2_DATA);

    outb(
        PIC1_COMMAND,
        ICW1_INIT |
            ICW1_ICW4);  // starts the initialization sequence (in cascade mode)
    io_wait();
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC1_DATA, 0x20);  // ICW2: Master PIC vector offset
    io_wait();
    outb(PIC2_DATA, 0x28);  // ICW2: Slave PIC vector offset
    io_wait();
    outb(PIC1_DATA, 4);  // ICW3: tell Master PIC that there is a slave PIC at
                         // IRQ2 (0000 0100)
    io_wait();
    outb(PIC2_DATA,
         2);  // ICW3: tell Slave PIC its cascade identity (0000 0010)
    io_wait();

    outb(PIC1_DATA,
         ICW4_8086);  // ICW4: have the PICs use 8086 mode (and not 8080 mode)
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();

    outb(PIC1_DATA, a1);  // restore saved masks.
    outb(PIC2_DATA, a2);
}
```

## 真正的中断处理程序intr_handlers
通过前面使用汇编实现的通用中断处理程序，在其中调用，intr_handlers转到c语言的函数来处理真正的中断，因此要实现一个中断处理程序，就只需要将intr_handlers这个数组对应的中断号下标赋值成所需要的函数即可。
```c
void register_intr_handler(int intr_no, intr_handler handler) {
    intr_handlers[intr_no] = handler;
}
```

为了能使所有的中断都能找到中断处理程序，需要为intr_handlers的每一项都赋个默认值
```c
void default_handler(uint32_t intr_no) {
    return;
}

extern intr_desc idt[INT_DESC_CNT];

void init_exception() {
    for (int i = 0; i < INT_DESC_CNT; i++) {
        register_intr_handler(i, default_handler);
    }
}
```
default_handler函数用于中断处理程序的默认值，当需要替换成真正的中断处理程序时，只需要调用register_intr_handler即可。

## 实现timer中断处理程序
cpu上有一个timer专门的芯片，用于对时钟产生的信息信号进行编程并产生相应的中断信息，因此也需要对timer的芯片做相应的编程初始化
```c
#define PIT_OSC_FREQ 1193182
#define OS_TICK_MS 10

#define PIT_CHANNEL0_DATA_PORT 0x40
#define PIT_COMMAND_MODE_PORT 0x43

#define PIT_CHANNLE0 (0 << 6)
#define PIT_LOAD_LOHI (3 << 4)
#define PIT_MODE0 (3 << 1)
void pit_init() {
    uint32_t reload_count = PIT_OSC_FREQ / (1000.0 / OS_TICK_MS);

    outb(PIT_COMMAND_MODE_PORT, PIT_CHANNLE0 | PIT_LOAD_LOHI | PIT_MODE0);
    outb(PIT_CHANNEL0_DATA_PORT, reload_count & 0xFF);
    outb(PIT_CHANNEL0_DATA_PORT, (reload_count >> 8) & 0xFF);
}
```
上面的代码中reload_count定义每秒产生的时间中断数，也就是时间中断的频率。

最后就是将时钟中断处理程序使用register_intr_handler函数注册到intr_handlers中。
```c
void init_timer() {
    register_intr_handler(INTR_NO_TIMER, timer_handler);
}
```
在真正的时钟中断处理函数中，调用任务调度函数schedule用于触发任务基于时间片的调度逻辑，具体实现参考[任务与调度](./task_schedule.md)
```c
void timer_handler(uint32_t intr_no) {
    schedule();
}
```

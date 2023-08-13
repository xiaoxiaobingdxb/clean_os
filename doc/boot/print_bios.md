# 使用bios的中断打印字符到屏幕

bios的10号中断0x0eh号子程序会将al寄存器中的asicii字符输出到屏幕，为了防止程序被重复执行导致重复打印，还需要在打印结束时添加一个原地跳转，相当于一个死循环。

最后在512字节的最后两字节输出0x55和0xaa，这个一个汇编程序编译生成二进制文件后用hex editor打开后可以看到最后的位置0x001f0上的最后是55和AA

```
.code16
.text
    .global _start

_start:
    mov $0, %ax
    mov %ax, %ds
    mov %ax, %ss
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    mov $_start, %esp

    mov $0xe, %ah
    mov $'c', %al
    int $0x10
    mov $'l', %al
    int $0x10
    mov $'e', %al
    int $0x10
    mov $'a', %al
    int $0x10
    mov $'n', %al
    int $0x10

    jmp .
.section boot_end, "ax"
.byte 0x55, 0xaa
```

这里为了方便在中间位置输出n个0，在连接脚本中定义了boot_end的位置，并且指定了代码段的位置

```
ld -Ttext=0x7c00 --section-start boot_end=0x7dfe
```

这样就能将boot_end定位到内存的0x7dfe，_start定义到0x7c00，

使用qemu的i386虚拟机执行生成的二进制文件，可在屏幕上显示clean这5个字符

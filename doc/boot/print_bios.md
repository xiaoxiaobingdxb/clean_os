# 使用bios的中断打印字符到屏幕

## 打印字符
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

## 打印字符串
bios的10号中断0x13号程序是以电传打字机的方式显示字符串

1. AH 0x13
2. AL 显示模式
   1. 0x00:字符串只包含字符码，显示之后不更新光标位置
   2. 0x01:字符串只包含字符码，显示 之后更新光标位置
   3. 0x02:字符串包含字符码和属性值，显示之后不更新光标位置
   4. 0x03:字符串包含字符码和属性值，显示之后更新光标位置
3. BH 视频页
4. BL 属性值
5. CX 字符串长度
6. DX 屏幕中字符串显示的起始行列位置
7. ES:BP 字符串数据的段:偏移地址

使用这个中断程序可以直接打印字符串而不再使用把字符串一个一个传入al后每个字符都调用一次中断，按照上面描述的寄存器功能将对应数据填入即可。这里将打印字符和打印字符串写到两个tag中，使用call方式调用
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

    mov $'c', %al
    call print_char
    mov $'l', %al
    call print_char
    mov $'e', %al
    call print_char
    mov $'a', %al
    call print_char
    mov $'n', %al
    call print_char

    mov $msgstr, %bp
    mov len, %cx
    call print_str

    jmp .

print_char: # char into al
    pusha
    mov $0x0e, %ah
    int $0x10
    popa
    ret

print_str: # str address into bp, str len into cx
    pusha
    mov $0x00, %al
    mov $0x13, %ah
    mov $0x01, %bx
    mov $0x0508, %dx
    int $0x10
    popa
    ret

msgstr: .asciz "Hello World!"
len: .int . - msgstr

.section boot_end, "ax"
.byte 0x55, 0xaa
```
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
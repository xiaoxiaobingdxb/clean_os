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
    push len
    push $msgstr
    call print_str
    add $0x04, %sp
    jmp .

msgstr: .string "loader..."
len: .int . - msgstr

#include "../common/bios_print.S"
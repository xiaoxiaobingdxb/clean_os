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
    call print_nl
    jmp loader_entry
    jmp .

msgstr: .string "loader..."
len: .int . - msgstr

#include "../common/bios_print.S"

    // 32位保护模式
    .code32
    .text
    .global protect_mode_entry
protect_mode_entry:
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %ss
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    jmp $8, $load_kernel
.code16
.text
    .global _start
    .extern boot_entry
_start:
    mov $0, %ax
    mov %ax, %ds
    mov %ax, %ss
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    mov $_start, %esp

    call clean_scene
    call reset_cursor

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
    mov $0x0100, %dx
    call print_str
    
    call read_disk
    jmp boot_entry
    ; mov $0x8000, %ax
    ; jmp %ax

    jmp .

msgstr: .string "Hello World!"
len: .int . - msgstr

#include "../common/bios_print.S"
#include "disk.S"

.section boot_end, "ax"
.byte 0x55, 0xaa

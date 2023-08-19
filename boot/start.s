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

    push $'s'
    push $'o'
    push $'-'
    push $'n'
    push $'a'
    push $'e'
    push $'l'
    push $'c'
    push $0x8
    call print_nchar
    add $0x12, %sp


    call print_nl

    push len
    push $msgstr
    call print_str
    add $0x04, %sp
    
    call print_nl
    
    call read_disk
    jmp boot_entry
    // mov $0x8000, %ax
    // jmp %ax

    jmp .

msgstr: .string "boot..."
len: .int . - msgstr

#include "../common/bios_print.S"
#include "disk.S"

.section boot_end, "ax"
.byte 0x55, 0xaa

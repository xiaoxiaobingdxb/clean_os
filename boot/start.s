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

.code16
.text
    .global _start
_start:
    mov $msgstr, %bp
    mov len, %cx
    mov $0x0200, %dx
    call print_str
    jmp .

msgstr: .string "loader..."
len: .int . - msgstr

#include "../common/bios_print.S"
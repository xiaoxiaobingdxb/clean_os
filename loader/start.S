.code16
.text
    .global _start
_start:
    mov $0x0e, %ah
    mov $'C', %al
    int $0x10
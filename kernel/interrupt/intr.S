.text
    .extern intr_handlers
    .global intr_entry_test
intr_entry_test:
    // if not error, push 0 to balance stack
    push $0

    // store all registers
    push %ds
    push %es
    push %fs
    push %gs
    pushal

    mov $0x20, %al
    outb %al, $0xa0
    outb %al, $0x20

    push $0x0
    mov 0x4(%esp), %ebx
    mov intr_handlers + 0x4 * 0x0, %eax
    call *%eax
    add $0x04, %esp
    popal
    pop %gs
    pop %fs
    pop %es
    pop %ds
    add $0x04, %esp // skip error code
    iret

    .data
    .global intr_entry_table
intr_entry_table:
.macro intr_entry intr_no, push_err_code
    .text
    .extern intr_handlers
intr_entry_\intr_no:
    .if \push_err_code > 0 // if not error, push 0 to balance stack
    push $0
    .endif

    // store all registers
    push %ds
    push %es
    push %fs
    push %gs
    pushal

    mov $0x20, %al
    outb %al, $0xa0
    outb %al, $0x20

    push $\intr_no
    mov intr_handlers + 0x4 * \intr_no, %eax
    call  *%eax
    jmp intr_exit

    .data
    .long intr_entry_\intr_no
.endm

    .text
    .global intr_exit
intr_exit:
    add $0x04, %esp
    popal
    pop %gs
    pop %fs
    pop %es
    pop %ds
    add $0x04, %esp // skip error code
    iret

    #include "intr_no.h"
intr_entry INTR_NO_DIVIDE,1 // divide zero
intr_entry 0x01,1
intr_entry 0x02,1
intr_entry 0x03,1 
intr_entry 0x04,1
intr_entry 0x05,1
intr_entry 0x06,1
intr_entry 0x07,1 
intr_entry 0x08,0
intr_entry 0x09,1
intr_entry 0x0a,0
intr_entry 0x0b,0 
intr_entry 0x0c,1
intr_entry 0x0d,0
intr_entry 0x0e,0
intr_entry 0x0f,1 
intr_entry 0x10,1
intr_entry 0x11,0
intr_entry 0x12,1
intr_entry 0x13,1 
intr_entry 0x14,1
intr_entry 0x15,1
intr_entry 0x16,1
intr_entry 0x17,1 
intr_entry 0x18,0
intr_entry 0x19,1
intr_entry 0x1a,0
intr_entry 0x1b,0 
intr_entry 0x1c,1
intr_entry 0x1d,0
intr_entry 0x1e,0
intr_entry 0x1f,1 
intr_entry INTR_NO_TIMER,1	// timer
intr_entry INTR_NO_KYB,1	// keyboard
intr_entry 0x22,1
intr_entry 0x23,1
intr_entry 0x24,1
intr_entry 0x25,1
intr_entry 0x26,1
intr_entry 0x27,1
intr_entry 0x28,1
intr_entry 0x29,1
intr_entry 0x2a,1
intr_entry 0x2b,1
intr_entry 0x2c,1
intr_entry 0x2d,1
intr_entry 0x2e,1
intr_entry 0x2f,1
#include "memory/config.h"
	.text
 	.global _start
_start:
    push 4(%esp)
    call kernel_init
    jmp $KERNEL_SELECTOR_CS, $gdt_reload

// after reset gdt and reload it, we should reset all segment registers using gdt segment selector
gdt_reload:
    mov $KERNEL_SELECTOR_DS, %ax
    mov %ax, %ds
    mov %ax, %ss
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    mov $(stack + KERNEL_STACK_SIZE), %esp
    and $0xfffff000, %esp // aligned to a page 
    jmp main // go to kernal main function, never back

    .bss
.comm stack, KERNEL_STACK_SIZE
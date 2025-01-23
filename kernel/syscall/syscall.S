    .text
    .extern syscall_table
    .global intr_entry_syscall
intr_entry_syscall:
    push $0

    // store all registers
    push %ds
    push %es
    push %fs
    push %gs
    pushal

    push $0x80

    push %edi
    push %esi
    push %edx
    push %ecx
    push %ebx

    // refresh cpu tlb
    push %eax
    mov %cr3, %eax
    mov %eax, %cr3
    pop %eax

    call  *syscall_table(,%eax, 4)
    add $0x14, %esp // pop ebx, ecx, edx
    mov $0x8, %ecx
    mov %eax, 0x0(%esp, %ecx, 4)	
    jmp intr_exit
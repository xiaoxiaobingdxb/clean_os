	.text
    .global switch_to
switch_to:
    // the next code address in stack top

    // store current task registers for thread_struct
    push %esi
    push %edi
    push %ebx
    push %ebp

    // store current task esp into first param
    // because the pcb store in current task stack
    mov 0x14(%esp), %eax // first param is cur task_struct pointer
    mov %esp, 0x0(%eax) // update current task thread_struct, thread_struct at the first and it's offset is 0

    // restore next task register value into registers
    mov 0x18(%esp), %eax // second param is next task_struct pointer
    mov 0x0(%eax), %esp //  restore next task pcb into stack

    // restore register value
    pop %ebp
    pop %ebx
    pop %edi
    pop %esi
    // here, the stack top is eip of next task
    // with c language function invoke rule, after switch_to return, next code address in eip, also is next task next code at last running
    ret
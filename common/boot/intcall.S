/**
* copy from linux, as it said, it is ugly, but it is also convenient
*/
	.code16
	.section ".inittext","ax"
	.globl	intcall
	.type	intcall, @function
intcall:
	/* Self-modify the INT instruction.  Ugly, but works. */
    push %ebp
    mov %esp, %ebp
    push %eax
    mov 0x08(%ebp), %eax
	cmpb	%al, 3f
	je	1f
	movb	%al, 3f
	jmp	1f		/* Synchronize pipeline */
1:
    pop %eax
	/* Save state */
	pushfl
	pushw	%fs
	pushw	%gs
	pushal

	/* Copy input state to stack frame */
	subw	$44, %sp
    mov	0x0c(%ebp), %si // point ireg address
	movw	%sp, %di
	movw	$11, %cx
	rep; movsl

	/* Pop full state from the stack */
	popal
	popw	%gs
	popw	%fs
	popw	%es
	popw	%ds
	popfl

	/* Actual INT */
	.byte	0xcd		/* INT opcode */
3:	.byte	0

	/* Push full state to the stack */
	pushfl
	pushw	%ds
	pushw	%es
	pushw	%fs
	pushw	%gs
	pushal

	/* Re-establish C environment invariants */
	cld
	movzwl	%sp, %esp
	movw	%cs, %ax
	movw	%ax, %ds
	movw	%ax, %es

	/* Copy output state from stack frame */
	mov	0x64(%esp), %edi // save state push 4*10, push full sate to the stack 4 * 11, 3rd argument offset 4 * (1+3), all is 100=0x64
	andw	%di, %di
	jz	4f
	movw	%sp, %si
	movw	$11, %cx
	rep; movsl
4:	addw	$44, %sp

	/* Restore state and return */
	popal
	popw	%gs
	popw	%fs
	popfl
    pop %ebp
	retl
	.size	intcall, .-intcall

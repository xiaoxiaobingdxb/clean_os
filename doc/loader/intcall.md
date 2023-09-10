# intcall
由于需要频繁地调用中断，而c语言是没有能力直接调用中断的，因此用c语言写实模式的代码时经常需要用内联汇编脚本的方式来调用中断，这样写的模板代码太多了。

## linux中的intcall
在参考linux源码的detect_memory时，发现在调用中断时用的是intcall，这个intcall的函数声明是用的c语言，但实现却是汇编语言。而且其非常方便使用，函数签名:`void intcall(u8 int_no, const struct biosregs *ireg, struct biosregs *oreg);`
其中参数的含义如下：
1. int_no:中断号
2. ireg:输入参数，包含了几乎所有的寄存器，这个输入参数的值会赋值给各个寄存器
3. oreg:输出参数，也包含了几乎所有的寄存器，在中断返回后，对应寄存器的值会mov给这个oreg对应的寄存器变量

ireg和oreg使用的类型都是biosregs，定义如下：
```c
struct biosregs {
	union {
		struct {
			uint32_t edi;
			uint32_t esi;
			uint32_t ebp;
			uint32_t _esp;
			uint32_t ebx;
			uint32_t edx;
			uint32_t ecx;
			uint32_t eax;
			uint32_t _fsgs;
			uint32_t _dses;
			uint32_t eflags;
		};
		struct {
			uint16_t di, hdi;
			uint16_t si, hsi;
			uint16_t bp, hbp;
			uint16_t _sp, _hsp;
			uint16_t bx, hbx;
			uint16_t dx, hdx;
			uint16_t cx, hcx;
			uint16_t ax, hax;
			uint16_t gs, fs;
			uint16_t es, ds;
			uint16_t flags, hflags;
		};
		struct {
			uint8_t dil, dih, edi2, edi3;
			uint8_t sil, sih, esi2, esi3;
			uint8_t bpl, bph, ebp2, ebp3;
			uint8_t _spl, _sph, _esp2, _esp3;
			uint8_t bl, bh, ebx2, ebx3;
			uint8_t dl, dh, edx2, edx3;
			uint8_t cl, ch, ecx2, ecx3;
			uint8_t al, ah, eax2, eax3;
		};
	};
};
```
这里使用了union，称为内联体，其跟struct的区别是union会将其包含的字段使用相同的内存，比如在biosregs的union中定义了三个struct，这三个struct将会共享内存。

第一个struct中第一个字段定义了edi表示32位寄存器，而第二个struct中定义了di和hdi两个16位寄存器，第三个struct中定义了dil、dih、edi2、edi3这四个8位寄存器，如果给edi赋值了，则相当于同时给di/hdi以及dil/dih/edi2/edi3这6个变量赋值了，因为这7个寄存器其实是同享了32位的内存。这跟真实的寄存器赋值不谋而合，比如在mov $0x12, %ax时，可以发现ah和al分别被赋值了1和2。用这种union的方式就可以让intcall在使用不同的中断时，可以复用参数类型，有些中断需要使用8位寄存器，有些要使用16位的而有些可能还要使用32位寄存器，而使用了union这种类型，就可以只用biosregs这一种类型。

注意到intcall函数的ireg和oreg的参数内存都指针类型，指针其实内存地址，指针变量内保存的内存起始地址。

## intcall的实现
linux源码中的intcall在gcc编译参数中设置了`-mregparm=3`，这个参数表示函数调用前三个参数使用寄存器传值，在[使用栈在汇编中实现函数调用](../boot/invoke_func.md)中介绍了，这种函数传参叫fastcall。这里为了更完整地体现intcall的调用细节，实现时最好采用完全栈传参，也就是stdcall，这样更能方便地观察寄存器的值变化情况，而所有的入参和出参均使用ireg和oreg来实现。

以下的intcall的汇编代码实现：
```c
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

```

现在来逐行解析一下intcall具体是怎么运行的
1. 首先是intcall标签里的代码：
   
   跟前面函数调用中一样，首先是把ebp的值压栈保存起来然后将esp的值赋值给ebp，这是为了方便用ebp进行寻址

   然后是把eax的值也压栈保存起来，并且寻址把ebp+0x08这个位置的值赋值给eax。这么操作的原因是在32位cpu中(这是因为gcc编译参数指定了-m32)函数传参stdcall的每个参数都是以32位进行压栈，因此在取数据的时候也要用eax这种32位的寄存器。这里寻址偏移是0x08的原因是ebp赋值是保存的esp的值，这时候esp表示栈顶指向call指令保存的eip的值，再加上为了保存ebp的值进行的压栈，因此为了在栈中取到call之前压栈的值，也就是函数的第一个参数的值，所以偏移就是0x08。这里比较重要的一点是，函数调用参数在压栈时是从右向左压，也就是最后一个参数在栈低，而第一个参数最后压栈在栈顶。

   将eax中的值也3标签处的值比较，如果不相等则将eax的值赋值给3标签处。之所以这么设计，可以看到3标签处默认就是个0，而3标签处前一个字节是0xcd，这2个字节的二进制所表示的指令其实就是int，因此在3标签的值其实就是int后面的参数，所以eax赋值给3标签就是为了给int传参，这个逻辑其实就是intcall的核心。

2. 然后是1这个标签内的代码：
   
   pop eax是为了将之前push eax的栈还原，然后的4行push是为了将当前寄存器进行压栈保存当前寄存器的现状，方便调用中断后将所有的寄存器的值还原。

   然后将ireg参数中的所有字段的值赋值给对应寄存器，这里使用了`rep; movsl`指令进行批量数据copy，对应的数据原地址保存在si中，数据目标地址只在在di中，数据量保存在cx中。由于biosregs这个struct中的所有寄存器加起来是44个字节(11个32位的寄存器)，因此首先需要将sp的值减去44次，相当于为这44个字节留出空为，然后将sp的值赋值给di，再将基于ebp的0x0c内存中的值赋值给si，这里的0x0c的偏移其实就是ireg，也就是第二个参数。在完成批量的数据copy后，再用对应的pop指令将刚才通过批量数据copy的数据通过pop的方式赋值给对应的寄存器，这里需要保证biosregs中字段的定义顺序跟pop的顺序完全一致。这里pop完成之后，会将sp加44，对应着前面将sp减44。

3. 再是3标签内的代码：
   
   前面讲到3标签处的数据其实就是int的参数，也就是中断号，在intcall中进行了赋值。中断执行完后再用push将所有的寄存器的值保存到栈中，这是为了方便将这些值再copy到输出参数oreg中。然后还原c环境下的寄存器的值，这是因为中断可能会修改部分段寄存器的值。

   最后又是跟ireg的值copy到栈中一样，这次是将栈中的值copy到oreg中，因此此时将0x64+esp的值赋值给di，将sp的值赋值给si，同样也是11个32位寄存器也就是44个字节，因此给cx赋值44，最后调用`rep; movsl`。这里为了避免oreg没有传值，也就是可能不传输出参数，所以用`andw	%di, %di;jz	4f`校验一下di是不是0，也就是oreg是不是0，如果是0表示传的值是NULL，这样也就没有必要将栈中的数据copy回oreg中了
   
   这里寻址的偏移之所以是64，是因为要寻址oreg，需要向栈顶找数据。前面在1标签中push了寄存器的值，是10个32位寄存器，也就是10*4。在调用中断后又用push保存寄存器的值，是11个32位寄存器，也就是11*4。最后是(1+3)*4，这里的1表示call指令将eip压栈，3表示第3个参数(注意ebp也压栈了，所以是3，不然用2就可以了)。因此最后是10*4+11*4+4*4=100=0x64

4. 最后是4标签中的代码：

4标签中的代码就比较简单了，由于在1标签中为了能够还原调用中断之前所有寄存器的值，所以将所有的寄存器的值进行了压栈，因此在最后还需要将前面压栈的数据再pop到对应的寄存器中。当然前面也对ebp进行了压栈，这里最后将ebp的值也pop出来。最后的最后再调用一个`retl`指令进行函数的返回，返回到函数调用处，并且将所有压栈的函数参数出栈，这样就是保证栈平衡了。

## intcall的思考
intcall确实是方便和中断的调用，可以像调用一个普通函数一样调用中断，并且参数输出和输出都很方便。但其实现方式却是将中断号赋值给代码段，这样虽然说可以实现给`int`指令传值，但一般来说编程是不允许修改代码段数据的，因为修改代码段数据可能会导致程序不正常执行，比如一个指令修改错误，可能会导致后续的指令都解析失败。正如linux中intcall实现中注释所写的那样，这种实现方式`it is ugly`，但确实是一种比较简单实现中断传值的方式了。

这里的核心思想其实就是代码即数据、数据即代码，我们可以像修改数据一样在代码运行时去修改代码，有点像运行时反射。但为了代码的可读性和安全性，在高级语言中不建议使用反射，在汇编中也不建议修改代码段。
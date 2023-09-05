# 使用栈在汇编中实现函数调用
## jmp指令实现跳转
在汇编中有一些结构化编程的方式，比如用flag定义一个过程，其实flag只是指定了这行执行的内存地址，使用`jmp flag`可以直接将flag的值mov到ip中，因此cpu接下来就会执行到flag指向的代码指令。这样flag定义一段代码指令实现上就可以当作高级语言中的函数，但高级语言中的函数执行完毕后还会继续执行调用函数下面的代码
```
int add(int a, int b) {
    return a + b;
}

int main() {
    int ret = add(1, 2);
    return 0;
}
```
比如上面的代码，在调用add函数后，还会执行return 0。但在汇编中使用`jmp flag`使ip调转到flag并执行完flag的代码后，还会继续向下执行而不是回到`jmp flag`这行代码。

## 汇编call/ret
汇编提供了call和ret这两个指令来实现类似于高级语言的函数调用与返回。`call flag`也会跟`jmp flag`一样将flag的值给到ip寄存器，相当于跳转到flag这行代码去，但为了实现函数调用后能够回到调用处，call指令还会将调用处的下一行代码指令的内存地址push到stack中，在函数执行完毕后使用ret指令让cpu回到调用处，相当于用ret指令将call指令push到stack中的调用处下一行代码的内存地址从stack中再pop出来，并将这个pop出来的地址赋值给ip，这样就能实现ret后能够回到调用处继续向下执行代码。
```
_start:
    mov $0x01, %ax
    call test
    mov %bx, %ax
test:
    mov $0x02, %bx
    ret

    mov $0x03, %ax
```
上面的代码`call test`指令会跳转到test处，开始执行`mov $0x02, %bx`这行代码，接着执行ret指令，这时又会回到调用处，并执行下一行代码`mov %bx, %ax`而不是去执行ret下面的代码。

## 寄存器值的保存
使用call/ret能够实现函数调用并返回调用处，但还存在一个问题，由于在flag的函数中，会使用一些寄存器，ret后这些寄存器使用的结果还是会保存，也就是说如果在flag中给修改过某些寄存器的值，ret后这些值还会保存，这将会导致原函数中的寄存器值丢失。
```
_start:
    mov $0x01, %ax
    call test
    mov %ax, %cx
test:
    mov $0x02, %ax
    mov %ax, %bx
    ret

    mov $0x03, %ax
```
上面的代码首先给ax中存入了0x01，在test中又给ax赋值0x02，然后ret后再把ax的值赋值给cx，这时cx中将会是0x02而不是0x01，这是因为在test中给ax赋值0x02这个结果ret后还存在，相当于ax在函数调用过程是引用类型的，或者说ax是个全局变量。很多时候在函数调用过程中我们不希望这样，因为函数调用可能是很多层，而寄存器在每层代码过程中都可能被修改。这样整个调用过程的值保存就会变得特别复杂。

为了让每次函数中每个寄存器使用后调用ret后能够像ip一样还原，也要将每个寄存器像call将ip保存到栈中一样，将每个寄存器原来的值push到stack中，在ret之前再pop出来，用这种办法改造上面的代码
```
_start:
    mov $0x01, %ax
    call test
    mov %ax, %cx
test:
    push %ax
    push %bx
    mov $0x02, %ax
    mov %ax, %bx
    pop %bx
    pop %ax
    ret

    mov $0x03, %ax
```
和之前的代码对比，仅仅加了两行push和两行pop，这里的push和pop只是模仿了call和ret，这样在ret后由于pop指令将值还原到了寄存器中。

## 函数调用的参数传递
前面的test flag实现的函数都是无参数的，但函数很多时候都会有参数传入和结果返回。解决了在函数调用后寄存器值还原问题后，为了实现真正的函数调用，还得解决参数传递的问题。考虑到call将ip push到stack后再在ret时将ip的值还原到ip，这样其实也相当于参数传递了，因此也可以使用stack来实现参数的传递。

由于要先push参数再调用call，并且在函数的开始还要将所有使用的寄存器的值也push到stack中保存起来，因此传入的参数实际上是在栈底的，在函数中也就不可能直接使用pop得到传入的参数了。为了解决stack中传入参数的访问，这里就需要用到寻地了，也就是根据内存地址访问到其值。例如实现add函数，需要传递两个参数，要使用到两个寄存器，再加上保存ip，因此真正的参数实际上保存的地址是sp+2*3，而一般sp是用来操作栈的，对于栈内的寻址一般使用bp寄存器，因此还得使用bp寄存器，bp寄存器的值也需要先push到stack中，所以实际上函数中显示地使用了三个寄存器并且隐式地使用了ip，一共使用了4个寄存器，真正的参数地址为bp+2*4，对于函数返回值的保存，一般是放到栈的最底部，这样的好处是在函数调用返回后，可以直接使用pop将函数返回值从栈中取出来。

还有一个问题，由于在调用前将参数压栈，函数返回后实际上这个传入参数还在栈上，因此还需要将这些值出栈，出栈使用pop指令需要使用一个寄存器来接收出栈的值，但实际上这些值我们已经不需要了，因此这样会浪费一个寄存器。为此，可以直接对sp进行`add $0x02, %sp`实现pop的效果。在所有入参都出栈后再用pop指令取出函数返回值。add函数的实现如下

```
_start:
    sub $0x02, %sp
    mov $0x01, %ax
    push %ax
    mov $0x02, %ax
    push %ax
    call add
    add $0x04, %sp
    pop %ax

add:
    push %ax
    push %bx
    push %bp
    mov %sp, %bp
    mov 0x08(%bp), %ax
    mov 0x0a(%bp), %bx
    add %bx, %ax
    mov %ax, 0x0c(%bp)
    pop %bp
    pop %bx
    pop %ax
    ret
```

在_start中：
1. 首先将sp的值-2，这是为函数的返回值储存留空位
2. 然后就是push传入的参数了
3. 接着调用add函数
4. 调用返回后再将sp的值+4，这是因为传入了两个参数，需要将这两个参数pop出来。这里使用的是16位寄存器，stack每次push/pop都是16位的，也就是2个字节，因此需要pop出两个入参就要将sp的值+4
5. 最后将函数的返回值再pop到ax中

不难看出，每次出stack的操作都有对应push和pop，对sp的add相当于pop，sub相当于push。这是为了保证一个子过程结束后sp的值不变，因为stack在子过程中实际上是被当作储存临时数据的工作，子过程结束后临时数据也应该被清空。就算不用清空，假设sp在子过程结束后变化了，那么最后ret时由于也要从stack中pop出call时的地址，也会是错误的，这样将导致调用函数后无法返回调用下。这个在高级语言中又叫作栈平衡。在高级语言中函数的参数和返回值，以及函数体内的局部变量，实际上都是储存在stack中的。stack实际上是个全局内存，因为每个函数都访问的stack都是同一段内存(这是因为ss相同)，就算是为了保存函数调用后不对stack这个全局内存产生附作用，也应该在push/sub后再用pop/add将stack的sp还原。

高级语言有时候也涉及到stack的原始操作，比如在代码生成时，例如java用代码生成在编译期来实现动态代理(实际上静态代码，因为代理是在编译期实现的，而不是运行期，只有运行期通过修改内存实现的代码才是真正意义上的动态代码)，如果在生成代码时操作stack没有保证stack的平衡，也会导致调用出问题，轻得取不到形参或返回值，重则进程直接异常中止

在add中：
1. 为了函数调用结束后能还原寄存器的值，先把所有要使用的寄存器的值push到stack
2. 将sp赋值给bp，用于使用bp来寻址
3. `mov offset(%bp), %xx`实现寻址，这里的`offset(%bp)`叫作基址寻址。bp叫作基址，offset叫作偏移，这里得到到真正地址addr=%bp+offset，基址保存在bp中，offset是偏移字节的个数，`0x08(%bp)`表示从bp位置向后再数8个字节得到的内存处的数据，同样由于使用的是16位寄存器，因此实际上是取出了两个字节的数据。由于bp在前面是用sp赋值的，而前面push了3个寄存器，再加上隐式地push了ip，所以要访问到调用call之前push到stack的传入的参数，需要用sp+4*2。由于先push的在stack底部，因此push的顺序实际上跟真正的函数形参的顺序是相反的，这也是为什么在高级语言中如果形参是一个计算式，那么都是从右边开始计算的。这样设计还有一个原因是：一般来说函数的参数顺序跟其重要性相关，也就是越前面的参数在逻辑上越重要，而越前面的参数越后push，所以会在栈的更顶部，在函数中通过基址寻址就会更方便，使用的offset就会更小。
4. 函数的返回值存入0x0c(%bp)处，这是因为返回值是用`sub $0x02, %sp`来留空位的，这是在push参数之前执行的，因此返回值实际上是在传入参数的下面
5. 把之前push的寄存器按倒序再pop出来，最后ret
   
## 函数传参的几种方式
前面讲到的函数传参和返回值都是基于stack的，这是最基本的方式。通常函数传参有以下这几种方式：
1. stdcall:也就是前面讲到的使用stack的实现，最基本的方式，也是大部分高级语言使用的方式
2. fastcall:部分参数通过寄存器传入和传出，其他参数通过stack传入。这是因为stack的push和pop需要操作内存，涉及到内存寻址和数据在内存和cpu之间传输，因此比直接使用寄存器更耗时和耗内存，但寄存器又是有限的，如果参数过多，则还是需要使用stack来传参
3. gcc regparam:在fastcall的基础上gcc可以指定参数有多少个使用寄存器
4. thiscall:在面向对象的语言中，会使用对象调用其方法，方法也是一个函数，因此会把this对象的地址当作第一个形参传入

## 内存寻址方式
内存寻址指的是给定内存地址，取出这个地址中的值。

直接寻址：看起来很简单，计算出内存地址，然后让地址总线去给定的地址取值即可，这叫作直接寻址。

基址寻址：但事情不会总是这么简单，如果要取连续内存中的多个值呢？如果用直接寻址，就需要每取一个值就计算一次地址，这样代码写起来复杂并且也浪费cpu，因此可以指定一个初始地址，每次取值时再指定一个偏移地址，这就跟之前用的`offset(%bp)`一样，这种寻址方式叫作基址寻址。

基址寻址每次只能隔2个字节取值，有时可能需要每隔n个字节取一个地址，比如数组，数组中每个元素的大小如果<=2个字节，则还可以用基址寻址，但如果元素的大小大于了2个字节则需要使用到变址寻址`base_addr(%ebp,%erax, size)`了，变址寻址是基址寻址的进化，其真实地址`addr=base_addr+%ebp+%erax*size`，例如在基于索引的数组元素访问时，base_addr表示数组元地址，ebp可以表示需要访问的数组起始元素，一般在n维数组中需要用到，erax表示需要访问的元素的index，size表示每个元素的大小，单位是字节

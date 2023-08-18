# 加载引导程序
上一节说到boot程序是cpu上电后由bios自动从第1个扇区读取到内存的0x7c00处，但这一个扇区只有512byte的数据，可能不够存下我们的所有os的代码，因此在boot的最后需要从磁盘读取其他扇区的代码数据并执行
## 读取磁盘
bios提供了0x13中断来读取硬盘数据，详细说明可见[Disk access using the BIOS (INT 13h)](https://wiki.osdev.org/Disk_access_using_the_BIOS_(INT_13h))

这里将读取磁盘的代码放到了read_disk这个flag下
```c
read_disk:
    pusha
    mov $0x8000, %bx	// 读取到的内存地址
	mov $0x2, %cx		// ch:磁道号,cl起始扇区号
	mov $0x2, %ah		// ah: 0x2读磁盘命令
	mov $64, %al		// al: 读取的扇区数量, 必须小于128,暂设置成32KB
	mov $0x0080, %dx	// dh: 磁头号,dl驱动器号0x80(磁盘1)
	int $0x13
    popa
    ret
```
bx寄存器指定了读取硬盘到内存中的起始地址

## 系统镜像文件
前面的boot子工程生成了boot.bin文件，直接用qemu加载运行了boot.bin文件，但这个文件中只包含了boot相关的代码和数据，也就是只有一个扇区的数据(512byte),用Hex Editor打开boot.bin可以看到最后一行是000001F0，最后结尾的十六进制数据是55AA。这里的1F0就表示的是1*16*16+15*16+0=496，这里是最后一行的起始地址，由于每行有16字节，因此boot.bin一共是512个字节,所以boot.bin其实是只有一个扇区的。由于read_disk要读取的loader程序在第二个扇区，因此需要做一个单独的系统镜像文件，将boot.bin和loader程序写入到这个系统镜像文件中，并且将loader的程序写入第二个扇区。

qemu提供了qemu-img程序用于创建系统镜像文件，创建出来的文件初始状态没有数据，也就是所有数据都是0。这里我们先创建一个64M的系统镜像文件`qemu-img create -f raw $os_img 64M`，os_img指定了生成的文件名，这里我们指定成clean_os.dmg。

## 多文件写入到镜像系统文件
linux或macos默认提供了dd命令用于转换和输出数据
`dd if=${binFiles[i]} of=$os_img bs=512 conv=notrunc seek=${binFiles[i+1]}`。这行命令表示输入文件为binFiles[i],输出文件为os_img表示的文件名，计入和输出的块大小为512个字节，也就是一个扇区，输出时不截断，seek表示输出时跳过多少个扇区，也就相当于指定输出到文件内的位置。

之前已经有boot工程并生成了boot.bin文件，这里还要生成一个loader文件，loader和boot工程几乎一样，只是不再需要在ld中指定section-start。

boot.bin写入到第1个扇区，loader.bin写入到第2个扇区，因此可以指定`binFiles=(boot.bin 0 loader.bin 1)`，这样可以在for循环中调用dd命令，后续如果还有其他文件有需要写入到系统镜像文件中，也就只需要在binFiles中添加即可。

## 工程构建脚本
到此，为了生成一个qemu能运行的系统镜像，需要做的工作：
1. 用cmake编译生成各个子工程的bin文件
2. 用qemu-img创建一个空的系统镜像文件
3. 用dd命令将各个子工程的bin文件写入到系统镜像文件

这多个步骤如果每次都需要手动敲命令的话也太过麻烦，因此可以将生成系统镜像的所有步骤都写入到一个build.sh的脚本中
```shell
# cmake
if [ ! -d "build" ];then
    mkdir ./build
fi
cd build
cmake ../
make

os_img=clean_os.dmg
# create os img if not exist
if [ ! -d "$os_img" ];then
    qemu-img create -f raw $os_img 64M
fi

binFiles=(boot.bin 0 loader.bin 1)
# write os data into img 
for (( i=0; i < ${#binFiles[@]}; i=i+2 ));do
    echo ${binFiles[i]}-${binFiles[i+1]}
    dd if=${binFiles[i]} of=$os_img bs=512 conv=notrunc seek=${binFiles[i+1]}
done
```

## 从boot到loader
前面的read_loader只是将loader代码从磁盘读到了内存的0x8000位置，为了执行loader的代码，还需要做一些操作。
这里就有两个办法了：一是直接用jmp跳到指定内存地址，这相当于是修改了ip寄存器的值，二是jmp到一个c函数中，在c函数中强loader的起始地址强转成一个函数并执行之。其他第二种方法跟第一种一样，因为c函数最终也是一个内存地址，执行一个函数也就是执行jmp指令。这里选用第二种方式，因为我们可能还需要用c代码做一些其他的事，从汇编先跳到c。

## 从汇编代码到c
gcc的汇编叫at&t语法，用.extern声明头文件中定义的函数名，这样就可以将这个函数名当作汇编的一个flag来使用了。使用这个方法，可以让gcc在编译汇编时，能够识别出c的函数，并且在链接时能够将c代码链接进最终文件，最终boot工程的start.S文件变成了这样:
```c
.code16
.text
    .global _start
    .extern boot_entry
_start:
    ......
    call read_disk
    jmp boot_entry
    ; mov $0x8000, %ax
    ; jmp %ax
```
为了能够跳转到loader，还需要用c实现boot_entry这个函数
```c
__asm__(".code16gcc");

#include "boot.h"
const int loader_addr = 0x8000;
void boot_entry() {
    ((void(*) (void))loader_add)();
}
```
上面的c代码中定义了一个常量，表示loader代码在内存中的起始地址，这个起始地址在loader工程的CMakeLists.txt中定义gcc链接命令时由-Ttext指定。在boot_entry函数中，将这个地址强转成一个无输入参数无输出参数的函数，并执行之。

为了验证是否成功跳转到了loader中，在loader工程的start.S代码中，使用print_str在屏幕的第二行打印"loader..."
```c
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
```
这里为了方便，将字符串的打印放到了common目录的bios_print.S文件中，这样就可以在boot和loader工程中都引入bios_print.S这个文件，实现汇编代码的复用了。
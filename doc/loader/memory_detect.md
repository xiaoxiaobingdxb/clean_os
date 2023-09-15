# 获取内存信息
在实模式中，可以通过段选择器+段内偏移实现内存访问，但在一般的操作系统内编程，使用的是保护模式，内存访问是通过分段+分页的方式来管理。不管是分段还是分页，都需要操作系统了解物理内存的分布情况。

## 物理内存
在8086架构的汇编编程中，cpu将各种硬件内存都映射成了可寻址的内存，这在x86架构的cpu也一样，因此内存信息应该还包含内存的类型

## 内存信息的查询方式
参考linux和osdev的说明，内存信息查询有以下三种常用方式
1. 15号中断的e820子程序<br>
   这种方式能拿到的内存信息最多，参考[osdev 15_e820](https://wiki.osdev.org/Detecting_Memory_(x86)#BIOS_Function:_INT_0x15.2C_EAX_.3D_0xE820)

   e820这种方式返回的数据在一个20Byte的内存中，分为5个字段，每个字节4个字节，这5个字段分别是起始地址的低32位和高32位、内存大小的低32位和高32位、内存的类型。因此可以将这内存定义成一个struct，这个struct包含了这5个字段，每个字段是一个32位的变量。
```c
   struct boot_e820_entry
{
    uint32_t low_addr;
    uint32_t high_addr;
    uint32_t low_size;
    uint32_t high_size;
    uint32_t type;
};
```
由于e820这种方式是按块的方式返回内存信息的，因此需要重复调用，每次在调用中断之前将返回数据中的ebx的数据赋值给ebx，这样的查询方式有点类似于分页查询。

2. 15号中断的e801子程序<br>
   参考[osdev 15_e801](https://wiki.osdev.org/Detecting_Memory_(x86)#BIOS_Function:_INT_0x15.2C_AX_.3D_0xE801)

   e801这种方式会有调用中断后cx中保存0~15M的内存大小，而如果内存大于15M则会在dx中保存更高位的，返回的数据表示的是内存的大小，单位是KB
   
3. 15号中断的88子程序<br>
   参考[osdev 15_88](https://wiki.osdev.org/Detecting_Memory_(x86)#BIOS_Function:_INT_0x15.2C_AH_.3D_0x88)

   88号子程序这种也只能返回1~63MB，返回的结果在保存ax中

## 内存信息传递给内核
为了将在实模式下查询到的内存信息传递给内核，需要将查询结果保存下来。这里选择将三种内存信息查询结果保存一个变量boot_params中，boot_params结构体就包含了三种方式的查询结果。
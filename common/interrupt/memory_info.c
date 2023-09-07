__asm__(".code16gcc");
#include "memory_info.h"
#include "../types/basic.h"
#include "..//boot/boot.h"

#define SMAP 0x534d4150 /* ASCII "SMAP" */
/** the best function to find memory info
 *the largest memory address is 2^64
 * @see osdev https://wiki.osdev.org/Detecting_Memory_(x86)#BIOS_Function:_INT_0x15.2C_EAX_.3D_0xE820
 */
static void detect_memory_15_e820()
{
	struct biosregs ireg, oreg;
	int count = 0;
	static struct boot_e820_entry buf;
	initregs(&ireg);
	initregs(&oreg);
	ireg.ax = 0xe820;
	ireg.cx = sizeof(buf);
	ireg.edx = SMAP;
	ireg.di = (size_t)&buf;
	do
	{
		intcall(0x15, &ireg, &oreg);
		ireg.ebx = oreg.ebx;

		if (oreg.eflags & X86_EFLAGS_CF)
			break;

		if (oreg.eax != SMAP)
		{
			count = 0;
			break;
		}
		// boot_params.e820_table[count++] = buf, this code is runtime error
		// so, assign the array value in this way
		boot_params.e820_table[count].low_addr = buf.low_addr;
		boot_params.e820_table[count].high_addr = buf.high_addr;
		boot_params.e820_table[count].low_size = buf.low_size;
		boot_params.e820_table[count].high_size = buf.high_size;
		boot_params.e820_table[count].type = buf.type;
		count++;
	} while (ireg.ebx && count < ARRAY_SIZE(boot_params.e820_table));
	boot_params.e820_entry_count = count;
}

/**
 * return 2 block
 * first is memory info 0~15MB
 * second is memory info 16MB~4GB
 * @see https://wiki.osdev.org/Detecting_Memory_(x86)#BIOS_Function:_INT_0x15.2C_AX_.3D_0xE801
 */
static void detect_memory_15_e801()
{
	struct biosregs ireg, oreg;
	initregs(&ireg);
	initregs(&oreg);
	ireg.ax = 0xe801;
	intcall(0x15, &ireg, &oreg);

	if (oreg.eflags & X86_EFLAGS_CF)
	{
		return;
	}
	if (oreg.cx > MB(15) / KB(1)) // this cx unit is KB
	{
		return; // only find 0~15M
	}
	else if (oreg.cx == MB(15) / KB(1))
	{
		boot_params.e801_k = (oreg.dx << 6) + oreg.cx;
	}
	else
	{
		boot_params.e801_k = oreg.cx;
	}
}

/**
 * only find memory info 1MB~63MB
 */
static void detect_memory_15_88()
{
	struct biosregs ireg, oreg;
	initregs(&ireg);
	initregs(&oreg);
	ireg.ah = 0x88;
	intcall(0x15, &ireg, &oreg);
	boot_params.ext_88_k = oreg.ax;
}

void detect_memory(void)
{
	detect_memory_15_e820();
	detect_memory_15_e801();
	detect_memory_15_88();
}
__asm__(".code16gcc");
#include "common/types/basic.h"
#include "common/bios_print.h"
#include "common/cpu/contrl.h"
#include "common/cpu/gdt.h"

#include "loader.h"
#include "common/boot/boot.h"
// #include "common/interrupt/memory_info.h"
#include "common/interrupt/memory_info.h"

uint16_t old_gdt_table[][4] = {
    {0, 0, 0, 0},
    {0xFFFF, 0x0000, 0x9A00, 0x00CF},
    {0xFFFF, 0x0000, 0x9200, 0x00CF},
};

void test_intcall()
{
    // test for intcall
    /*
    struct biosregs ir, or ;
    struct biosregs *ireg = &ir, *oreg = &or;
    initregs(ireg);
    initregs(oreg);
    ireg->ah = 0x3;
    uint8_t intNo = 0x10;
    initregs(ireg);
    initregs(oreg);
    ireg->ah = 0x0e;
    ireg->al = 'L';
    intcall(0x10, ireg, oreg);

    initregs(ireg);
    initregs(oreg);
    ireg->ah = 0x03;
    intcall(0x10, ireg, oreg);
    */
}
struct boot_params boot_params;
void loader_entry()
{
    // test_intcall();
    detect_memory();
    struct boot_params *p = &boot_params;
    print_str("from 16bit into 32bit");
    print_nl();
    // 1. close interrupt
    cli();
    // 2. open A20
    uint8_t v = inb(0x92);
    outb(0x92, v | 0x02);
    // 3. load gdt
    lgdt((uint32_t)old_gdt_table, sizeof(old_gdt_table));
    // 4. open cr0
    uint32_t cr0 = read_cr0();
    write_cr0(cr0 | 1);
    // 5. jmp far to clear pipeline
    jmp_far(8, (uint32_t)protect_mode_entry);
}
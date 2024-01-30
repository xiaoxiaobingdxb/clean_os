#include "common/cpu/gdt.h"
#include "common/lib/string.h"
#include "config.h"
#include "mem.h"

#define SEG_G (1 << 15)
#define SEG_D (1 << 14)

#define SEG_P_PRESENT (1 << 7)

#define SEG_DPL0 (0 << 5)
#define SEG_DPL3 (3 << 5)

#define SEG_S_SYSTEM (0 << 4)
#define SEG_S_NORMAL (1 << 4)

#define SEG_TYPE_CODE (1 << 3)
#define SEG_TYPE_DATA (0 << 3)

#define SEG_TYPE_RW (1 << 1)

#pragma pack(1)
typedef struct segment_desc_t {
    uint16_t limit15_0;
    uint16_t base15_0;
    uint8_t base23_16;
    uint16_t attr;
    uint8_t base31_24;
} segment_desc;
#pragma pack()

static segment_desc gdt_table[GDT_TABLE_SIZE];
void gdt_set(int select, uint32_t base, uint32_t limit, uint16_t attr) {
    segment_desc *desc = gdt_table + (select >> 3);
    if (limit > 0xfffff) {
        attr |= SEG_G;
        limit /= KB(4);
    }
    desc->limit15_0 = limit & 0xffff;
    desc->base15_0 = base & 0xffff;
    desc->base23_16 = (base >> 16) & 0xff;
    desc->base31_24 = (base >> 24) & 0xff;
    desc->attr = attr | (((limit >> 16) & 0xf) << 8);
}

/**
 * @see https://wiki.osdev.org/Task_State_Segment
 */
typedef struct {
    uint32_t link;

    uint32_t esp0;
    uint32_t ss0;

    uint32_t esp1;
    uint32_t ss1;

    uint32_t esp2;
    uint32_t ssp2;

    uint32_t cr3;

    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;

    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldtr;
    uint32_t trap;
    uint32_t iopb;
} tss_t;

tss_t tss;

void update_tss_esp(uint32_t esp) { tss.esp0 = esp; }

void init_tss() {
    memset(&tss, 0, sizeof(tss));
    tss.ss0 = KERNEL_SELECTOR_SS;
    tss.iopb = sizeof(tss);
}

void init_gdt() {
    // clear all in gdt
    for (int i = 0; i < GDT_TABLE_SIZE; i++) {
        gdt_set(i << 3, 0, 0, 0);
    }
    init_tss();
    uint32_t USER_MEM = 0x80000000;
    gdt_set(KERNEL_SELECTOR_DS, 0x00000000, 0xFFFFFFFF,
            SEG_P_PRESENT | SEG_DPL0 | SEG_S_NORMAL | SEG_TYPE_DATA |
                SEG_TYPE_RW | SEG_D | SEG_G);
    gdt_set(KERNEL_SELECTOR_CS, 0x00000000, 0xFFFFFFFF,
            SEG_P_PRESENT | SEG_DPL0 | SEG_S_NORMAL | SEG_TYPE_CODE |
                SEG_TYPE_RW | SEG_D | SEG_G);
    gdt_set(USER_SELECTOR_DS, 0x00000000, 0xFFFFFFFF,
            SEG_P_PRESENT | SEG_DPL3 | SEG_S_NORMAL | SEG_TYPE_DATA |
                SEG_TYPE_RW | SEG_D | SEG_G);
    gdt_set(USER_SELECTOR_CS, 0x00000000, 0xFFFFFFFF,
            SEG_P_PRESENT | SEG_DPL3 | SEG_S_NORMAL | SEG_TYPE_CODE |
                SEG_TYPE_RW | SEG_D | SEG_G);
    gdt_set(TSS_SELECTOR, (uint32_t)&tss, (uint32_t)sizeof(tss) - 1,
            SEG_P_PRESENT | SEG_DPL0 | SEG_S_SYSTEM | 0x9 | (0 << 14) | (0 << 15));
    lgdt((uint32_t)gdt_table, sizeof(gdt_table));
    ltr(TSS_SELECTOR);
}
#include "common/cpu/gdt.h"
#include "mem.h"
#include "config.h"

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

#define SEG_RPL0 (0 << 0)
#define SEG_RPL3 (3 << 0)

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

void init_gdt() {
    // clear all in gdt
    for (int i = 0; i < GDT_TABLE_SIZE; i++) {
        gdt_set(i << 3, 0, 0, 0);
    }
    gdt_set(KERNEL_SELECTOR_DS, 0x00000000, 0xFFFFFFFF,
            SEG_P_PRESENT | SEG_DPL0 | SEG_S_NORMAL | SEG_TYPE_DATA |
                SEG_TYPE_RW | SEG_D | SEG_G);
    gdt_set(KERNEL_SELECTOR_CS, 0x00000000, 0xFFFFFFFF,
            SEG_P_PRESENT | SEG_DPL0 | SEG_S_NORMAL | SEG_TYPE_CODE |
                SEG_TYPE_RW | SEG_D | SEG_G);
    lgdt((uint32_t)gdt_table, sizeof(gdt_table));
}
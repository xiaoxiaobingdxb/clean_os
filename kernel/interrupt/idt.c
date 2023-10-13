#include "idt.h"

#include "../memory/config.h"

intr_desc idt[INT_DESC_CNT];

void make_intr_desc(intr_desc *desc, uint8_t attr, intr_handler entry) {
    desc->zero = 0;
    desc->entry_low15 = (uint32_t)entry & 0xffff;
    desc->entry_high15_31 = ((uint32_t)entry >> 16) & 0xffff;
    desc->attr = attr;
    desc->selector = KERNEL_SELECTOR_CS;
}

void intr_entry_test();

void init_intr() {
    for (int i = 0; i < INT_DESC_CNT; i++) {
        make_intr_desc(idt + i, IDT_DESC_ATTR_DPL0, intr_entry_table[i]);
    }
    // make_intr_desc(idt, IDT_DESC_ATTR_DPL0, intr_entry_test);
}
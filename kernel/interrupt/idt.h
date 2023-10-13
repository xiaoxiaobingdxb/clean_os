#ifndef INTERRUPT_IDT_H
#define INTERRUPT_IDT_H

#include "common/types/basic.h"

/**
 * @see https://wiki.osdev.org/IDT#Table
 */

#define INT_DESC_CNT 0x81  // 129

#define GATE_TYPE_TASK 0x5
#define GATE_TYPE_INT_16 0x6
#define GATE_TYPE_TRAP_16 0x7
#define GATE_TYPE_INT_32 0xE
#define GATE_TYPE_TRAP_32 0xF

#define IDT_DESC_P 1
#define IDT_DESC_DPL0 0
#define IDT_DESC_DPL3 3
#define IDT_DESC_32_TYPE GATE_TYPE_INT_32

#define IDT_DESC_ATTR_DPL0 \
    ((IDT_DESC_P << 7) + (IDT_DESC_DPL0 << 5) + IDT_DESC_32_TYPE)
#define IDT_DESC_ATTR_DPL3 \
    ((IDT_DESC_P << 7) + (IDT_DESC_DPL3 << 5) + IDT_DESC_32_TYPE)

typedef struct {
    uint16_t entry_low15;
    uint16_t selector;
    uint8_t zero;
    uint8_t attr;
    uint16_t entry_high15_31;
} intr_desc;

typedef void* intr_handler;

extern intr_handler intr_entry_table[INT_DESC_CNT];

void init_intr();

static inline void lidt(uint32_t start, uint32_t size) {
    struct {
        uint16_t limit;
        uint16_t start15_0;
        uint16_t start31_16;
    } idt;

    idt.start31_16 = start >> 16;
    idt.start15_0 = start & 0xFFFF;
    idt.limit = size - 1;

    __asm__ __volatile__("lidt %[i]" ::[i] "m"(idt));
}
void make_intr_desc(intr_desc *desc, uint8_t attr, intr_handler entry);

#endif
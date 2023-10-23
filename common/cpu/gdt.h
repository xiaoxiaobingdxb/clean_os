#ifndef CPU_GDT_H
#define CPU_GDT_H
#include "../types/basic.h"

static inline void lgdt(uint32_t start, uint32_t size) {
    struct {
        uint16_t limit;
        uint16_t start15_0;
        uint16_t start31_15;
    } gdt;

    gdt.start31_15 = start >> 16;
    gdt.start15_0 = start & 0xffff;
    gdt.limit = size - 1;
    __asm__ __volatile__("lgdt %[g]" ::[g] "m"(gdt));
}

static inline void ltr(int selector) {
    __asm__ __volatile__("ltr %%ax" ::"a"(selector));
}

#endif
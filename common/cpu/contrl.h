#ifndef CPU_CONTROL_H
#define CPU_CONTROL_H
#include "../types/basic.h"

// disable cpu interrupt
void cli() {
    __asm__ __volatile__("cli");
}
// enable cpu interrupt
void sti() {
    __asm__ __volatile__("sti");
}

uint8_t inb(uint16_t port) {
    uint8_t rv;
    __asm__ __volatile__("inb %[p], %[v]":[v]"=a"(rv):[p]"d"(port));
    return rv;
}

void outb(uint16_t port, uint8_t v) {
    __asm__ __volatile__("outb %[v], %[p]"::[p]"d"(port), [v]"a"(v));
}

uint32_t read_cr0() {
    uint32_t cr0;
    __asm__ __volatile__("mov %%cr0, %[v]":[v]"=r"(cr0));
    return cr0;
}

void write_cr0(uint32_t v) {
    __asm__ __volatile__("mov %[v], %%cr0"::[v]"r"(v));
}

void jmp_far(uint32_t selector, uint32_t offset) {
    uint32_t addr[] = {offset, selector};
    __asm__ __volatile__("ljmpl *(%[a])"::[a]"r"(addr));
}

#endif
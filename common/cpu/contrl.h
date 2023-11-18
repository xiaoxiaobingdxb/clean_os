#ifndef CPU_CONTROL_H
#define CPU_CONTROL_H
#include "../types/basic.h"

// disable cpu interrupt
static inline void cli() { __asm__ __volatile__("cli"); }
// enable cpu interrupt
static inline void sti() { __asm__ __volatile__("sti"); }

typedef uint32_t eflags_t;

static inline eflags_t read_eflags() {
    eflags_t eflags;
    asm("pushfl\n\t"
        "pop %[v]"
        : [v] "=r"(eflags));
    return eflags;
}

static inline void write_elfags(eflags_t eflags) {
    asm("push %[v] \n\t"
        "popfl" ::[v] "r"(eflags));
}
static inline eflags_t enter_intr_protect() {
    eflags_t state = read_eflags();
    cli();
    return state;
}

static inline void leave_intr_protect(eflags_t state) {
    write_elfags(state);
}

static inline uint8_t inb(uint16_t port) {
    uint8_t rv;
    __asm__ __volatile__("inb %[p], %[v]" : [v] "=a"(rv) : [p] "d"(port));
    return rv;
}

static inline void outb(uint16_t port, uint8_t v) {
    __asm__ __volatile__("outb %[v], %[p]" ::[p] "d"(port), [v] "a"(v));
}

static inline void io_wait(void) { outb(0x80, 0); }

static inline uint16_t inw(uint16_t port) {
    uint16_t rv;
    __asm__ __volatile__("inw %[p], %[v]" : [v] "=a"(rv) : [p] "d"(port));
    return rv;
}

static inline uint32_t read_cr0() {
    uint32_t cr0;
    __asm__ __volatile__("mov %%cr0, %[v]" : [v] "=r"(cr0));
    return cr0;
}

static inline void write_cr0(uint32_t v) {
    __asm__ __volatile__("mov %[v], %%cr0" ::[v] "r"(v));
}

static inline uint32_t read_cr3() {
    uint32_t cr3;
    __asm__ __volatile__("mov %%cr3, %[v]" : [v] "=r"(cr3));
    return cr3;
}

static inline void write_cr3(uint32_t v) {
    __asm__ __volatile__("mov %[v], %%cr3" ::[v] "r"(v));
}

static inline uint32_t read_cr4() {
    uint32_t cr4;
    __asm__ __volatile__("mov %%cr4, %[v]" : [v] "=r"(cr4));
    return cr4;
}

static inline void write_cr4(uint32_t v) {
    __asm__ __volatile__("mov %[v], %%cr4" ::[v] "r"(v));
}

static inline void jmp_far(uint32_t selector, uint32_t offset) {
    uint32_t addr[] = {offset, selector};
    __asm__ __volatile__("ljmpl *(%[a])" ::[a] "r"(addr));
}

static inline void hlt() { __asm__ __volatile__("hlt"); }

static inline void die(int code) {
    for (;;)
        ;
}

#endif
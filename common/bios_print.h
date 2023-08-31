#ifndef BIOS_PRINT_H
#define BIOS_PRINT_H

static inline void print_str(const char *str) {
    char c;
    while ((c = *str++) != '\0') {
        __asm__ __volatile__(
            "push %%ax\n\t"
            "mov $0x0e, %%ah\n\t"
            "mov %[ch], %%al\n\t"
            "int $0x10\n\t"
            "pop %%ax"::[ch]"r"(c)
        );
    }
}

static inline void print_nl() {
    print_str("\n\r");
}

#endif
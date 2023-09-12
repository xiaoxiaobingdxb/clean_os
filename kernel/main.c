#include "common/cpu/gdt.h"
void main() {
    uint32_t a = 10;
    uint32_t *p = &a;
    set_page();
    p = (uint32_t*)0x80000000;
    uint32_t value = *p;
    for (;;) {
    }
}
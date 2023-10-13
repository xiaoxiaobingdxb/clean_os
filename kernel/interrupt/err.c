#include "idt.h"
#include "intr.h"
#include "err.h"

void default_handler(uint32_t intr_no) {
    return;
}

extern intr_desc idt[INT_DESC_CNT];

void init_exception() {
    for (int i = 0; i < INT_DESC_CNT; i++) {
        register_intr_handler(i, default_handler);
    }
}
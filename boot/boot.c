__asm__(".code16gcc");

#include "boot.h"
const int loader_addr = 0x8000;
void boot_entry() {
    ((void(*) (void))loader_addr)();
}

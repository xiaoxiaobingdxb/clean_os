#include "common/bios_print.h"
#include "common/cpu/gdt.h"
#include "common/elf/elf.h"
#include "common/interrupt/memory_info.h"
#include "common/io/disk.h"
#include "common/types/basic.h"
#include "loader.h"
#include "common/cpu/mem_page.h"

#define SYS_KERNEL_LOAD_ADDR MB(1)

void load_kernel() {
//    int elf_size = MB(1);
//    int sector_size = 512;
//    int sector_count = elf_size / sector_size + (elf_size % sector_size > 0 ? 1 : 0);
//    uint32_t  addr = SYS_KERNEL_LOAD_ADDR + sector_count * sector_size;
//    read_disk(100, sector_count, (uint8_t *) addr);
//
//    uint32_t kernel_entry = reload_elf_file((uint8_t *) addr);
//    enable_page_mode();
//    ((void (*)(struct boot_params *)) kernel_entry)(&boot_params);
//    for (;;) {
//    }
    read_disk(100, 500, (uint8_t *)SYS_KERNEL_LOAD_ADDR);
    uint32_t kernel_entry = reload_elf_file((uint8_t *)SYS_KERNEL_LOAD_ADDR);
    enable_page_mode();
    ((void (*)(struct boot_params *))kernel_entry)(&boot_params);
    for (;;) {
    }
}
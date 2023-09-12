#include "../interrupt/memory_info.h"
#include "contrl.h"
#include "gdt.h"

#define PDE_P (1 << 0)
#define PDE_PS (1 << 7)
#define PDE_W (1 << 1)
#define PDE_U (1 << 2)
#define CR4_PSE (1 << 4)
#define CR0_PG (1 << 31)
#define MAP_ADDR 0x80000000

void enable_page_mode(void) {
    // 使用4MB页块，这样构造页表就简单很多，只需要1个表即可。
    // 以下表为临时使用，用于帮助内核正常运行，在内核运行起来之后，将重新设置
    static uint32_t page_dir[1024] __attribute__((aligned(KB(4)))) = {
        [0] = PDE_P | PDE_PS | PDE_W,  // PDE_PS，开启4MB的页
    };
    // 设置PSE，以便启用4M的页，而不是4KB
    uint32_t cr4 = read_cr4();
    write_cr4(cr4 | CR4_PSE);

    // 设置页表地址
    write_cr3((uint32_t)page_dir);

    // 开启分页机制
    write_cr0(read_cr0() | CR0_PG);
}

uint8_t map_pyh_buff[KB(4)] __attribute__((aligned(KB(4)))) = {0x36};

void set_page() {
    static uint32_t page_dir2[KB(1)] __attribute__((aligned(KB(4)))) = {0};

    page_dir2[MAP_ADDR >> 12 & 0x3ff] =
        (uint32_t)map_pyh_buff | PDE_P | PDE_W | PDE_U;

    uint32_t *page_dir = (uint32_t *)read_cr3();
    page_dir[MAP_ADDR >> 22] = (uint32_t)page_dir2 | PDE_P | PDE_W | PDE_U;
}
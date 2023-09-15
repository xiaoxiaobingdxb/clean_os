#include "mem_page.h"

#include "../boot/string.h"
#include "../interrupt/memory_info.h"
#include "../tool/lib.h"
#include "contrl.h"
#include "gdt.h"
#include "mmu.h"

#define PDE_P (1 << 0)
#define PDE_PS (1 << 7)
#define PDE_W (1 << 1)
#define PDE_U (1 << 2)
#define CR4_PSE (1 << 4)
#define CR0_PG (1 << 31)

#define PTE_P (1 << 0)
#define PTE_W (1 << 1)
#define PTE_U (1 << 2)

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
#define MAP_ADDR 0x80000000

    static uint32_t page_dir2[KB(1)] __attribute__((aligned(KB(4)))) = {0};

    page_dir2[MAP_ADDR >> 12 & 0x3ff] =
        (uint32_t)map_pyh_buff | PDE_P | PDE_W | PDE_U;

    uint32_t *p = page_dir2;

    uint32_t *page_dir = (uint32_t *)read_cr3();
    page_dir[MAP_ADDR >> 22] = (uint32_t)page_dir2 | PDE_P | PDE_W | PDE_U;
}
uint32_t page_position = MB(4);
// alloc some page from physical memory
// return physical memeory address
uint32_t alloc_phy_mem(int page_count) {
    page_position += MEM_PAGE_SIZE;
    return page_position;
}

pte_t *find_pte(pde_t *page_dir, uint32_t vaddr, int alloc) {
    pte_t *pte;
    pde_t *pde = page_dir + pde_index(vaddr);
    if (pde->present) {
        pte = (pte_t *)pe_phy_addr(pde->phy_pt_addr);
    } else {
        if (!alloc) {
            return (pte_t *)0;
        }
        // alloc a page from physical memeory
        uint32_t phy_addr = alloc_phy_mem(1);
        if (phy_addr == 0) {
            return (pte_t *)0;
        }
        pde->v = phy_addr | PTE_P | PTE_W | PTE_U;
        pte = (pte_t *)phy_addr;
        memset((void *)phy_addr, 0, MEM_PAGE_SIZE);
    }
    return pte + pte_index(vaddr);
}

// map count page from vaddr(virtual memory) into paddr(physical memory)
int mem_create_map(pde_t *page_dir, uint32_t vaddr, uint32_t paddr, int count,
                   uint32_t perm) {
    for (int i = 0; i < count; i++) {
        pte_t *pte = find_pte(page_dir, vaddr, 1);
        if (pte == (pte_t *)0) {  // pte not found
            return -1;
        }
        if (pte->present != 0) {
            return -1;
        }
        pte->v = paddr | perm | PTE_P;
        vaddr += MEM_PAGE_SIZE;
        paddr += MEM_PAGE_SIZE;
    }
    return 0;
}

int get_phy_addr(uint32_t vaddr, uint32_t *paddr, int alloc) {
    pde_t *page_dir = (pde_t *)read_cr3();
    pte_t *pte = find_pte(page_dir, vaddr, 1);
    if (pte == 0) {
        return -1;
    }
    if (pte->present == 0) {
        if (!alloc) {
            return -1;
        }
        uint32_t phy_addr = alloc_phy_mem(1);
        if (phy_addr == 0) {
            return -1;
        }
        int err = mem_create_map(page_dir, vaddr, phy_addr, 1, PTE_W | PTE_U);
    }
    if (paddr != 0) {
        *paddr = pe_phy_addr(pte->phy_page_addr);
    }
    return 0;
}

int set_byte(uint32_t vaddr, uint8_t value, int alloc) {
    int err = get_phy_addr(vaddr, NULL, alloc);
    if (err != 0) {
        return err;
    }
    *((uint8_t *)vaddr) = value;
    return 0;
}
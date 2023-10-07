#ifndef CPU_MEM_PAGE_H
#define CPU_MEM_PAGE_H

#include "../interrupt/memory_info.h"
#include "mmu.h"

#define MEM_PAGE_SIZE KB(4)
void enable_page_mode(void);
void set_page_dir(uint32_t page_dir);
uint32_t read_page_dir();
void set_page();
int set_byte(uint32_t vaddr, uint8_t value, int alloc);
int get_phy_addr(uint32_t vaddr, uint32_t *paddr, int alloc);
pte_t *find_pte(pde_t *page_dir, uint32_t vaddr, int alloc);
#endif
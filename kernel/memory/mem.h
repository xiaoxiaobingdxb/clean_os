#ifndef MEMORY_MEM_H
#define MEMORY_MEM_H

#include "common/interrupt/memory_info.h"
#include "common/types/basic.h"

#define PTE_P (1 << 0)
#define PTE_W (1 << 1)
#define PTE_U (1 << 2)

void init_gdt();

void init_paging(uint64_t all_mem);

void init_mem(struct boot_params *params);

uint32_t current_page_dir();

/**
 * @brief create virtual page dir for user
 * @return the new page dir address in physical memory
 * if < 0 is error
 * else the physical address
 */
uint32_t create_uvm();

/**
 * @brief free all physical memory for the page_dir
 */
void destroy_uvm(uint32_t page_dir);

/**
 * @brief alloc virtual memory page from page_dir
 * @param[in] page_dir pde table point or firt pde pointer
 * @param[in] vstart virtual memory start address
 * @param[in] size virtual memory length, unit byte
 * @param[in] perm virtual memory permission
 * @return 0 if success  otherwise < 0
 */
int alloc_vm_for_page_dir(uint32_t page_dir, uint32_t vstart, uint32_t size,
                          int perm);

/**
 * @brief free virtual memory page from page_dir, also to free physical memory,
 * to free physical memory alloc, and delete pte in page table
 */
int free_vm_for_page_dir(uint32_t page_dir, uint32_t vstart, uint32_t size);

#endif
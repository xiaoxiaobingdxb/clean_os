#ifndef MEMORY_MEM_H
#define MEMORY_MEM_H

#include "common/cpu/mem_page.h"
#include "common/cpu/mmu.h"
#include "common/interrupt/memory_info.h"
#include "common/lib/bitmap.h"
#include "common/types/basic.h"

#define PTE_P (1 << 0)
#define PTE_W (1 << 1)
#define PTE_U (1 << 2)

#define PDE_CNT 1024
#define PTE_CNT 1024

struct addr_alloc_t_ {
    uint32_t start;
    uint32_t page_size;
    bitmap_t bitmap;
};
typedef struct addr_alloc_t_ phy_addr_alloc_t;
typedef struct addr_alloc_t_ vir_addr_alloc_t;

uint32_t vaddr2paddr(uint32_t page_dir, uint32_t vaddr);

void init_gdt();

void update_tss_esp(uint32_t esp);

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

uint32_t alloc_kernel_mem(int page_count);

/**
 * @brief free page_count memory from vaddr in kernel
*/
int unalloc_kernel_mem(uint32_t vaddr, int page_count);

/**
 * @brief alloc_mem is to alloc memery from vir_addr_alloc and phy_addr_alloc,
 * but not map virtual memory into physical memory
 * @return if success 0 else < 0
 */
// int alloc_mem(vir_addr_alloc_t *vir_addr_alloc, int page_count,
            //    uint32_t addrs[2]);

/**
 * @brief alloc_mem_page is to alloc continuous virutal memory and alloc discontinuous physical memory
 * @param vir_addr_alloc
 * @param page_count
 * @param vaddr out param allocated continuous virutal memory
 * @param paddrs out param allocated discontinuous virtual memory, so it is a array
 * @return if success 0 else < 0
*/
int alloc_mem_page(vir_addr_alloc_t *vir_addr_alloc, int page_count, uint32_t *vaddr, uint32_t *paddrs);

/**
 * @brief reply memory to virtual memory and physical memory
 */
int unalloc_mem(uint32_t page_dir, vir_addr_alloc_t *vir_addr_alloc,
                uint32_t vaddr, uint32_t page_count);

/**
 * @brief map vaddr(virtual memory) into paddr(physical memory) with
 * page_count(4K) vaddr and paddr if not aligned to a page(4K), it will be
 * aligned to a page
 */
uint32_t map_mem(uint32_t page_dir, uint32_t vaddr, uint32_t paddr, int page_count,
            bool override);

uint32_t map_mem_page(uint32_t page_dir, uint32_t vaddr, uint32_t *paddrs, int page_count, bool override);

/**
 * @brief remove paging memory from virtual memory to physical memory
 */
int unmap_mem(uint32_t page_dir, uint32_t vaddr, int page_count);

/**
 * @brief malloc virtual/physical memory and paging map virtual memory to
 * physical memory
 * @return start of allocated the vitual memory, if not aligned to a page(4K), it will be
 * aligned to a page
 */
uint32_t malloc_mem(uint32_t page_dir, vir_addr_alloc_t *vir_addr_alloc,
                    int page_count);
/**
 * @brief malloc virtual/physical memory at vaddr in vitual memory
 * @return start of allocated the vitual memory, if not aligned to a page(4K), it will be
 * aligned to a page
 */
uint32_t malloc_mem_vaddr(uint32_t page_dir, vir_addr_alloc_t *vir_addr_alloc,
                          uint32_t vaddr, int page_count);

/**
 * free virtual/physical memory and remove paing map
 */
int unmalloc_mem(uint32_t page_dir, vir_addr_alloc_t *vir_addr_alloc,
                 uint32_t vaddr, int page_count);

void init_user_vir_addr_alloc(vir_addr_alloc_t *alloc);

uint32_t count_mem_used(vir_addr_alloc_t *alloc);

#endif
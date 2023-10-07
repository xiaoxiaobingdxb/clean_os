#include "common/cpu/mem_page.h"
#include "common/lib/bitmap.h"
#include "common/lib/string.h"
#include "common/tool/math.h"
#include "common/types/basic.h"
#include "common/types/byte.h"
#include "config.h"
#include "mem.h"

#define PDE_CNT 1024
#define PTE_CNT 1024

#define PDE_P (1 << 0)
#define PDE_PS (1 << 7)
#define PDE_W (1 << 1)
#define PDE_U (1 << 2)
#define CR4_PSE (1 << 4)
#define CR0_PG (1 << 31)

typedef struct {
    uint32_t start;
    uint32_t page_size;
    bitmap_t bitmap;
} phy_addr_alloc_t;

static phy_addr_alloc_t kernel_phy_addr_alloc;
static phy_addr_alloc_t user_phy_addr_alloc;

void init_phy_addr_alloc(phy_addr_alloc_t *phy_addr_alloc, uint8_t *bits,
                         uint32_t start, uint32_t size, uint32_t page_size) {
    phy_addr_alloc->bitmap.bits = bits;
    uint32_t bit_size = size / page_size;
    phy_addr_alloc->bitmap.bytes_len = (bit_size + 7) / 8;
    bitmap_init(&phy_addr_alloc->bitmap);
    phy_addr_alloc->start = start;
    phy_addr_alloc->page_size = page_size;
}

uint32_t alloc_phy_mem(phy_addr_alloc_t *phy_addr_alloc, int page_count) {
    int bit_idx = bitmap_scan(&phy_addr_alloc->bitmap, page_count);
    if (bit_idx < 0) {
        return -1;
    }
    for (int i = 0; i < page_count; i++) {
        bitmap_set(&phy_addr_alloc->bitmap, bit_idx + i, 1);
    }
    return phy_addr_alloc->start + bit_idx * phy_addr_alloc->page_size;
}

phy_addr_alloc_t *get_phy_addr_alloc(uint32_t paddr) {
    phy_addr_alloc_t *phy_addr_alloc = &user_phy_addr_alloc;
    if (paddr < phy_addr_alloc->start) {
        phy_addr_alloc = &kernel_phy_addr_alloc;
    }
    return phy_addr_alloc;
}

int free_phy_mem(uint32_t paddr, int page_count) {
    phy_addr_alloc_t *phy_addr_alloc = get_phy_addr_alloc(paddr);
    if (!phy_addr_alloc) {
        return -1;
    }
    uint32_t bit_idx =
        (paddr - phy_addr_alloc->start) / phy_addr_alloc->page_size;
    for (int i = 0; i < page_count; i++) {
        bitmap_set(&phy_addr_alloc->bitmap, bit_idx + i, 0);
    }
    return 0;
}

typedef struct {
    void *vstart;
    void *vend;
    void *pstart;
    uint32_t perm;
} mem_map;

static pde_t kernel_page_dir[PDE_CNT] __attribute__((aligned(MEM_PAGE_SIZE)));

pte_t *find_pte_alloc(phy_addr_alloc_t *phy_addr_alloc, pde_t *page_dir,
                      uint32_t vaddr, int alloc) {
    pte_t *pte;
    pde_t *pde = page_dir + pde_index(vaddr);
    if (pde->present) {
        pte = (pte_t *)pe_phy_addr(pde->phy_pt_addr);
    } else {
        if (!alloc) {
            return (pte_t *)0;
        }
        // alloc a page from physical memeory
        uint32_t phy_addr = alloc_phy_mem(phy_addr_alloc, 1);
        if (phy_addr == 0) {
            return (pte_t *)0;
        }
        pde->v = phy_addr | PDE_P | PDE_W;
        pte = (pte_t *)phy_addr;
        memset((void *)phy_addr, 0, MEM_PAGE_SIZE);
    }
    return pte + pte_index(vaddr);
}

// map vaddr of virtual memory addr into paddr of physical memory addr
int create_mem_map(pde_t *page_dir, uint32_t vaddr, uint32_t paddr,
                   int page_count, uint32_t perm) {
    for (int i = 0; i < page_count; i++) {
        // phy_addr_alloc_t *phy_addr_alloc = get_phy_addr_alloc(paddr);
        // if (!phy_addr_alloc) {
        //     return -1;
        // }
        pte_t *pte = find_pte_alloc(&kernel_phy_addr_alloc, page_dir, vaddr, 1);
        if (pte == (pte_t *)0) {
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

// map vaddr of virtual memory addr into paddr of physical memory addr, but uint
// 4M
int create_big_mem_map(pde_t *page_dir, uint32_t vaddr, uint32_t size,
                       uint32_t paddr, uint32_t perm) {
    uint32_t offset_n4m = vaddr % MB(4);
    if (offset_n4m > 0) {  // use 4k paging map
        int err = create_mem_map(page_dir, vaddr, paddr,
                                 (MB(4) - offset_n4m) / MEM_PAGE_SIZE, perm);
        if (err) {
            return err;
        }
        uint32_t handle_size = MB(4) - offset_n4m;
        vaddr += handle_size;
        paddr += handle_size;
        size -= handle_size;
    }

    for (int i = 0; i < up2(size, MB(4)) / MB(4); i++) {
        int pde_idx = pde_index(vaddr);
        pde_t *pde = page_dir + pde_idx;
        pde->v = paddr | PDE_P | PDE_PS | perm;
        vaddr += MB(4);
        paddr += MB(4);
    }
}

#define EBDA_START 0x00080000
#define MEM_EXT_START MB(1)

static uint64_t phy_all_mem;
static uint64_t kernel_all_memory;

void init_paging(uint64_t all_mem) {
    phy_all_mem = all_mem;
    extern uint8_t mem_free_start[];

    uint8_t *mem_free = (uint8_t *)mem_free_start;

    kernel_all_memory = phy_all_mem / KERNEL_MEM_SIZE_RATE;
    if (kernel_all_memory > GB(1)) {  // kernel use 1GB max
        kernel_all_memory = GB(1);
    }
    uint64_t user_all_memory = phy_all_mem - kernel_all_memory;

    uint32_t mem_up1m_free = kernel_all_memory - MEM_EXT_START;
    mem_up1m_free = down2(mem_up1m_free, MEM_PAGE_SIZE);
    init_phy_addr_alloc(&kernel_phy_addr_alloc, mem_free, MEM_EXT_START,
                        mem_up1m_free, MEM_PAGE_SIZE);

    // user bitmap is set after kernel bitmap, both all is 4G/4K/8=128K
    init_phy_addr_alloc(&user_phy_addr_alloc,
                        mem_free + kernel_phy_addr_alloc.bitmap.bytes_len,
                        kernel_all_memory, user_all_memory, MEM_PAGE_SIZE);

    extern uint8_t s_text[], e_text[], s_data[], e_data[];
    extern uint8_t kernel_base[];
    uint32_t kernel_vstart = MEM_EXT_START;
    uint32_t kernel_vend = kernel_all_memory - 1;
    mem_map kernel_map[] = {
        {kernel_base, s_text, 0, PTE_W},                    // kernel stack seg
        {s_text, e_text, s_text, 0},                        // kernel code seg
        {s_data, (void *)(EBDA_START - 1), s_data, PTE_W},  // kernel data seg
        {(void *)kernel_vstart, (void *)kernel_vend, (void *)MEM_EXT_START,
         PTE_U | PTE_W},  //
    };
    memset(kernel_page_dir, 0, sizeof(kernel_page_dir));
    for (int i = 0; i < sizeof(kernel_map) / sizeof(mem_map); i++) {
        mem_map *kmap = kernel_map + i;
        if ((uint32_t)kmap->vstart >= kernel_vstart &&
            (uint32_t)kmap->vend <= kernel_vend) {
            // real kernel memory, use big virtual memory map
            uint32_t vstart = (uint32_t)kmap->vstart,
                     vend = (uint32_t)kmap->vend;
            create_big_mem_map(kernel_page_dir, vstart, vend - vstart + 1,
                               (uint32_t)kmap->pstart, kmap->perm);
            continue;
        }
        uint32_t vstart = down2((uint32_t)kmap->vstart, MEM_PAGE_SIZE);
        uint32_t vend = up2((uint32_t)kmap->vend, MEM_PAGE_SIZE);
        int p_count = (vend - vstart) / MEM_PAGE_SIZE;
        if (p_count > 0) {
            create_mem_map(kernel_page_dir, vstart, (uint32_t)kmap->pstart,
                           p_count, kmap->perm);
        }
    }
    set_page_dir((uint32_t)kernel_page_dir);
}

uint32_t current_page_dir() { return read_page_dir(); }

/**
 * @details the step is:
 * 1. alloc a page from physical memory for store page_dir
 * 2. clear all pde
 * 3. copy shared pde from kernel page_dir into user page_dir
 */
uint32_t create_uvm() {
    uint32_t phy_addr = alloc_phy_mem(&kernel_phy_addr_alloc, 1);
    if (phy_addr < 0) {
        return -1;
    }
    pde_t *page_dir = (pde_t *)phy_addr;
    memset((void *)phy_addr, 0, MEM_PAGE_SIZE);
    for (int i = pde_index(kernel_all_memory) - 1; i >= 0; i--) {
        if ((kernel_page_dir + i)->present) {
            (page_dir + i)->v = (kernel_page_dir + i)->v;
        }
    }
    return phy_addr;
}

/**
 * @details free all physical memory for page_dir, include all pde
 */
void destroy_uvm(uint32_t page_dir) {
    pde_t *pde = (pde_t *)page_dir;
    int user_pde_start = pde_index(kernel_all_memory);
    for (int i = user_pde_start; i < PDE_CNT; i++) {
        pde += i;
        if (!pde->present) {
            continue;
        }
        pte_t *pte = (pte_t *)pe_phy_addr(pde->phy_pt_addr);
        if (!pte) {
            continue;
        }
        for (int i = 0; i < PTE_CNT; i++) {
            pte += i;
            if (!pte->present) {
                continue;
            }
            free_phy_mem(pe_phy_addr(pte->phy_page_addr), 1);  // free a page
        }
        free_phy_mem(pe_phy_addr(pde->phy_pt_addr), 1);  // free page table
    }
    free_phy_mem(page_dir, 1);  // free page directory
}

/**
 * @details alloc physical memory for the virtual memory and map them
 * if the size isn't page_size, up to paeg_size to avoid fragment
 */
int alloc_vm_for_page_dir(uint32_t page_dir, uint32_t vstart, uint32_t size,
                          int perm) {
    uint32_t page_count = up2(size, MEM_PAGE_SIZE) / MEM_PAGE_SIZE;
    // alloc physical memory every time a page for using the fragment physical
    // memory
    for (int i = 0; i < page_count; i++) {
        uint32_t paddr = alloc_phy_mem(&user_phy_addr_alloc, 1);
        if (!paddr) {
            return -1;
        }
        int err = create_mem_map((pde_t *)page_dir, vstart, paddr, 1, perm);
        if (err) {
            // free paddr for alloced
            free_phy_mem(paddr, 1);
            return err;
        }
        vstart += MEM_PAGE_SIZE;
    }
    return 0;
}

pde_t *get_pde(uint32_t page_dir, uint32_t vaddr) {
    pde_t *pde = (pde_t *)page_dir;
    if (!pde) {
        return (pde_t *)NULL;
    }
    return pde + pde_index(vaddr);
}

pte_t *get_pte(uint32_t page_dir, uint32_t vaddr) {
    pde_t *pde = get_pde(page_dir, vaddr);
    if (!pde || !pde->present) {
        return (pte_t *)NULL;
    }
    pte_t *pte = (pte_t *)pe_phy_addr(pde->phy_pt_addr);
    if (!pte) {
        return (pte_t *)NULL;
    }
    return pte + pte_index(vaddr);
}

/**
 * @details using paging memory management, virtual address high 10 bit is the
 * index in page directory, middle 10 bit is the index in page table, the
 * physical stored in pte value
 * 1. using vaddr high 10 bit get the index in page directory to get page table
 * address
 * 2. convert page table address into pte_t pointer, and using vaddr middle 10
 * bit get the index in page table to get page start address
 * 3. vaddr last 12 bit is the offset with page start address, get physical
 * using page start address add the offset
 */
uint32_t vaddr2paddr(uint32_t page_dir, uint32_t vaddr) {
    pte_t *pte = get_pte(page_dir, vaddr);
    if (!pte || !pte->present) {
        return -1;
    }
    return phy_addr(pe_phy_addr(pte->phy_page_addr), vaddr);
}

int free_vm_for_page_dir(uint32_t page_dir, uint32_t vstart, uint32_t size) {
    uint32_t page_count = up2(size, MEM_PAGE_SIZE) / MEM_PAGE_SIZE;
    // virtual memory is continuous but the physical memory may not be
    // continuous,so we need to free one page by on page
    for (int i = 0; i < page_count; i++) {
        uint32_t paddr = vaddr2paddr(page_dir, vstart);
        if (!paddr) {
            return -1;
        }
        int err = free_phy_mem(paddr, 1);
        if (!err) {
            return err;
        }
        pte_t *pte = get_pte(page_dir, vstart);
        if (!pte || !pte->present) {
            continue;
        }
        pte->v = 0;
        vstart += MEM_PAGE_SIZE;
    }
    return 0;
}

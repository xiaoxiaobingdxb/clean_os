#include "common/cpu/mem_page.h"
#include "common/lib/bitmap.h"
#include "common/lib/string.h"
#include "common/tool/math.h"
#include "common/types/basic.h"
#include "common/types/byte.h"
#include "config.h"
#include "mem.h"

#define PDE_P (1 << 0)
#define PDE_PS (1 << 7)
#define PDE_W (1 << 1)
#define PDE_U (1 << 2)
#define CR4_PSE (1 << 4)
#define CR0_PG (1 << 31)

#define MALLOC_MAX_PAGE 1024

phy_addr_alloc_t kernel_phy_addr_alloc;
vir_addr_alloc_t kernel_vir_addr_alloc;
phy_addr_alloc_t user_phy_addr_alloc;

pde_t kernel_page_dir[PDE_CNT] __attribute__((aligned(MEM_PAGE_SIZE)));

void init_addr_alloc(struct addr_alloc_t_ *addr_alloc, uint8_t *bits,
                     uint32_t start, uint32_t size, uint32_t page_size) {
    addr_alloc->bitmap.bits = bits;
    uint32_t bit_size = size / page_size;
    addr_alloc->bitmap.bytes_len = (bit_size + 7) / 8;
    bitmap_init(&addr_alloc->bitmap);
    addr_alloc->start = start;
    addr_alloc->page_size = page_size;
}

phy_addr_alloc_t *get_phy_addr_alloc(uint32_t paddr) {
    phy_addr_alloc_t *phy_addr_alloc = &user_phy_addr_alloc;
    if (paddr < phy_addr_alloc->start) {
        phy_addr_alloc = &kernel_phy_addr_alloc;
    }
    return phy_addr_alloc;
}

void alloc_mem_use(struct addr_alloc_t_ *addr_alloc, uint32_t addr,
                   int page_count, uint8_t use) {
    int bit_idx = (addr - addr_alloc->start) / addr_alloc->page_size;
    for (int i = 0; i < page_count; i++) {
        bitmap_set(&addr_alloc->bitmap, bit_idx + i, use);
    }
}

int free_phy_mem(uint32_t paddr, int page_count) {
    phy_addr_alloc_t *phy_addr_alloc = get_phy_addr_alloc(paddr);
    if (!phy_addr_alloc) {
        return -1;
    }
    alloc_mem_use(phy_addr_alloc, paddr, page_count, 0);
    return 0;
}

int free_vir_mem(vir_addr_alloc_t *vir_addr_alloc, uint32_t vaddr,
                 int page_count) {
    alloc_mem_use(vir_addr_alloc, vaddr, page_count, 0);
    return 0;
}

uint32_t alloc_phy_mem(phy_addr_alloc_t *phy_addr_alloc, int page_count) {
    int bit_idx = bitmap_scan(&phy_addr_alloc->bitmap, page_count);
    if (bit_idx < 0) {
        return -1;
    }
    uint32_t addr = phy_addr_alloc->start + bit_idx * phy_addr_alloc->page_size;
    alloc_mem_use(phy_addr_alloc, addr, page_count, 1);
    return addr;
}

int alloc_phy_mem_page(phy_addr_alloc_t *phy_addr_alloc, int page_count,
                       uint32_t *paddrs) {
    if (page_count > MALLOC_MAX_PAGE) {
        return -1;
    }
    for (int i = 0; i < page_count; i++) {
        uint32_t paddr = alloc_phy_mem(phy_addr_alloc, 1);
        if (paddr < 0) {  // if err, free all allocated memory
            for (int j = 0; j < i; j++) {
                free_phy_mem(paddrs[j], 1);
            }
            return -1;
        }
        paddrs[i] = paddr;
    }
    return 0;
}

uint32_t alloc_vir_mem(vir_addr_alloc_t *vir_addr_alloc, int page_count) {
    int bit_idx = bitmap_scan(&vir_addr_alloc->bitmap, page_count);
    if (bit_idx < 0) {
        return -1;
    }
    uint32_t addr = vir_addr_alloc->start + bit_idx * vir_addr_alloc->page_size;
    alloc_mem_use(vir_addr_alloc, addr, page_count, 1);
    return addr;
}

/*
int alloc_mem(vir_addr_alloc_t *vir_addr_alloc, int page_count,
              uint32_t addrs[2]) {
    if (vir_addr_alloc->bitmap.bits == kernel_vir_addr_alloc.bitmap.bits) {
        uint32_t addr = alloc_phy_mem(&kernel_phy_addr_alloc, page_count);
        if (addr >= 0) {
            alloc_mem_use(&kernel_vir_addr_alloc, addr, page_count, 1);
            addrs[0] = addrs[1] = addr;
        } else {
            return -1;
        }
    } else {
        addrs[0] = alloc_phy_mem(&user_phy_addr_alloc, page_count);
        if (addrs[0] >= 0) {
            addrs[1] = alloc_vir_mem(vir_addr_alloc, page_count);
        } else {
            return -1;
        }
    }
    return 0;
}
*/

int alloc_mem_page(vir_addr_alloc_t *vir_addr_alloc, int page_count,
                   uint32_t *vaddr, uint32_t *paddrs) {
    phy_addr_alloc_t *phy_addr_alloc = &user_phy_addr_alloc;
    if (vir_addr_alloc->bitmap.bits == kernel_vir_addr_alloc.bitmap.bits) {
        phy_addr_alloc = &kernel_phy_addr_alloc;
    }
    uint32_t vaddr_start = alloc_vir_mem(vir_addr_alloc, page_count);
    if (vaddr_start < 0) {
        return -1;
    }
    *vaddr = vaddr_start;
    for (int i = 0; i < page_count; i++) {
        uint32_t paddr = alloc_phy_mem(phy_addr_alloc, 1);
        if (paddr < 0) {  // if err, to free all already virtual/physical memory
            free_vir_mem(vir_addr_alloc, vaddr_start, page_count);
            for (int j = 0; j < i; j++) {
                free_phy_mem(paddrs[j], 1);
            }
            return -1;
        }
        // if success
        paddrs[i] = paddr;
    }
    return 0;
}

uint32_t alloc_kernel_mem(int page_count) {
    return malloc_mem((uint32_t)kernel_page_dir, &kernel_vir_addr_alloc,
                      page_count);
}

int unalloc_kernel_mem(uint32_t vaddr, int page_count) {
    return unmalloc_mem((uint32_t)kernel_page_dir, &kernel_vir_addr_alloc,
                        vaddr, page_count);
}

int unalloc_mem(uint32_t page_dir, vir_addr_alloc_t *vir_addr_alloc,
                uint32_t vaddr, uint32_t page_count) {
    bool have = false;
    uint32_t paddr = vaddr2paddr(page_dir, vaddr, &have);
    if (!have) {
        return -1;
    }
    int err = free_phy_mem(paddr, page_count);
    if (!err) {
        err = free_vir_mem(vir_addr_alloc, vaddr, page_count);
    }
    return err;
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
uint32_t vaddr2paddr(uint32_t page_dir, uint32_t vaddr, bool *have) {
    pte_t *pte = get_pte(page_dir, vaddr);
    if (!pte || !pte->present) {
        *have = false;
        return -1;
    }
    *have = true;
    return phy_addr(pe_phy_addr(pte->phy_page_addr), vaddr);
}

typedef struct {
    void *vstart;
    void *vend;
    void *pstart;
    uint32_t perm;
} mem_map;

pte_t *find_pte_alloc(pde_t *page_dir, uint32_t vaddr, int alloc, int permit) {
    pte_t *pte;
    pde_t *pde = page_dir + pde_index(vaddr);
    if (pde->present) {
        pte = (pte_t *)pe_phy_addr(pde->phy_pt_addr);
    } else {
        if (!alloc) {
            return (pte_t *)0;
        }
        // alloc a page from kernel memeory
        uint32_t addr = alloc_kernel_mem(1);
        if (addr < 0) {
            return (pte_t *)0;
        }
        pde->v = addr | PDE_P | PDE_W | PDE_U;
        pte = (pte_t *)addr;
        memset((void *)addr, 0, MEM_PAGE_SIZE);
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
        pte_t *pte = find_pte_alloc(page_dir, vaddr, 1, perm);
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

uint64_t phy_all_mem;
uint64_t kernel_all_memory;
uint64_t user_all_memory;

void init_paging(uint64_t all_mem) {
    phy_all_mem = all_mem;
    extern uint8_t mem_free_start[];

    uint8_t *mem_free = (uint8_t *)mem_free_start;

    kernel_all_memory = phy_all_mem / KERNEL_MEM_SIZE_RATE;
    if (kernel_all_memory > GB(1)) {  // kernel use 1GB max
        kernel_all_memory = GB(1);
    }
    user_all_memory = phy_all_mem - kernel_all_memory;

    uint32_t mem_up1m_free = kernel_all_memory - MEM_EXT_START;
    mem_up1m_free = down2(mem_up1m_free, MEM_PAGE_SIZE);
    init_addr_alloc(&kernel_phy_addr_alloc, mem_free, MEM_EXT_START,
                    mem_up1m_free, MEM_PAGE_SIZE);

    init_addr_alloc(&kernel_vir_addr_alloc,
                    mem_free + kernel_phy_addr_alloc.bitmap.bytes_len,
                    MEM_EXT_START, mem_up1m_free, MEM_PAGE_SIZE);

    // user bitmap is set after kernel bitmap, both all is 4G/4K/8=128K
    init_addr_alloc(&user_phy_addr_alloc,
                    mem_free + kernel_phy_addr_alloc.bitmap.bytes_len +
                        kernel_vir_addr_alloc.bitmap.bytes_len,
                    kernel_all_memory, user_all_memory, MEM_PAGE_SIZE);

    extern uint8_t s_text[], e_text[], s_data[], e_data[];
    extern uint8_t kernel_base[];
    uint32_t kernel_vstart = MEM_EXT_START;
    uint32_t kernel_vend = kernel_all_memory - 1;
    mem_map kernel_map[] = {
        {kernel_base, s_text, 0, PTE_U | PTE_W},  // kernel stack seg
        {s_text, e_text, s_text, PTE_U | PTE_W},  // kernel code seg
        {s_data, (void *)(EBDA_START - 1), s_data,
         PTE_U | PTE_W},  // kernel data seg
        {(void *)EBDA_START, (void *)(kernel_vstart - 1), (void *)EBDA_START,
         PTE_U | PTE_W},
        {(void *)kernel_vstart, (void *)kernel_vend, (void *)MEM_EXT_START,
         PTE_U | PTE_W},
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
    uint32_t addr = alloc_kernel_mem(1);
    if (addr < 0) {
        return 0;
    }
    pde_t *page_dir = (pde_t *)addr;
    memset((void *)addr, 0, MEM_PAGE_SIZE);
    for (int i = pde_index(kernel_all_memory) - 1; i >= 0; i--) {
        if ((kernel_page_dir + i)->present) {
            (page_dir + i)->v = (kernel_page_dir + i)->v;
        }
    }
    return addr;
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
    phy_addr_alloc_t *phy_addr_alloc = &user_phy_addr_alloc;
    if ((pde_t *)page_dir == kernel_page_dir) {
        phy_addr_alloc = &kernel_phy_addr_alloc;
    }
    for (int i = 0; i < page_count; i++) {
        uint32_t paddr = alloc_phy_mem(phy_addr_alloc, 1);
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

int free_vm_for_page_dir(uint32_t page_dir, uint32_t vstart, uint32_t size) {
    uint32_t page_count = up2(size, MEM_PAGE_SIZE) / MEM_PAGE_SIZE;
    // virtual memory is continuous but the physical memory may not be
    // continuous,so we need to free one page by on page
    for (int i = 0; i < page_count; i++) {
        bool have = false;
        uint32_t paddr = vaddr2paddr(page_dir, vstart, &have);
        if (!have) {
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

uint32_t map_mem(uint32_t page_dir, uint32_t vaddr, uint32_t paddr,
                 int page_count, bool override) {
    vaddr = down2(vaddr, MEM_PAGE_SIZE);
    uint32_t ret_vaddr = vaddr;
    paddr = down2(paddr, MEM_PAGE_SIZE);
    for (int i = 0; i < page_count; i++) {
        bool have = false;
        uint32_t get_paddr = vaddr2paddr(page_dir, vaddr, &have);
        if (have && !override) {  // already map
            return -1;
        }
        int permit = PTE_W;
        if ((pde_t *)page_dir != kernel_page_dir) {
            permit |= PTE_U;
        }
        int err = create_mem_map((pde_t *)page_dir, vaddr, paddr, 1, permit);
        if (err) {
            return -1;
        }
        vaddr += MEM_PAGE_SIZE;
        paddr += MEM_PAGE_SIZE;
    }
    return ret_vaddr;
}

uint32_t map_mem_page(uint32_t page_dir, uint32_t vaddr, uint32_t *paddrs,
                      int page_count, bool override) {
    if (page_count > MALLOC_MAX_PAGE) {
        return -1;
    }
    vaddr = down2(vaddr, MEM_PAGE_SIZE);
    uint32_t ret_vaddr = vaddr;
    for (int i = 0; i < page_count; i++) {
        uint32_t addr = map_mem(page_dir, vaddr, paddrs[i], 1, override);
        if (addr < 0) {
            for (int j = 0; j < i; j++) {
                unmap_mem(page_dir, ret_vaddr + j * MEM_PAGE_SIZE, 1);
            }
            return -1;
        }
        vaddr += MEM_PAGE_SIZE;
    }
    return ret_vaddr;
}

int unmap_mem(uint32_t page_dir, uint32_t vaddr, int page_count) {
    vaddr = down2(vaddr, MEM_PAGE_SIZE);
    for (int i = 0; i < page_count; i++) {
        pte_t *pte = find_pte((pde_t *)page_dir, vaddr, 0);
        if (pte) {
            pte->present = !PTE_P;  // only reset present field
        }
        vaddr += MEM_PAGE_SIZE;
    }
}

uint32_t paddrs[MALLOC_MAX_PAGE];
uint32_t malloc_mem(uint32_t page_dir, vir_addr_alloc_t *vir_addr_alloc,
                    int page_count) {
    if (page_count > MALLOC_MAX_PAGE) {
        return -1;
    }
    uint32_t vaddr;
    int err = alloc_mem_page(vir_addr_alloc, page_count, &vaddr, paddrs);
    if (err) {
        return -1;
    }
    // kernel memory dose not need to map
    if (vir_addr_alloc != &kernel_vir_addr_alloc) {
        return map_mem_page(page_dir, vaddr, paddrs, page_count, 1);
    }
    return vaddr;
}

uint32_t malloc_mem_vaddr(uint32_t page_dir, vir_addr_alloc_t *vir_addr_alloc,
                          uint32_t vaddr, int page_count) {
    vaddr = down2(vaddr, MEM_PAGE_SIZE);  // aligned a page(4K)
    phy_addr_alloc_t *phy_addr_alloc = &user_phy_addr_alloc;
    if (vir_addr_alloc->bitmap.bits == kernel_vir_addr_alloc.bitmap.bits) {
        phy_addr_alloc = &kernel_phy_addr_alloc;
    }
    for (int i = 0; i < page_count; i++) {
        pte_t *pte = get_pte(page_dir, vaddr + i * MEM_PAGE_SIZE);
        if (pte != NULL &&
            pte->present) {  // virtual memory had used to be allocated
            return -1;
        }
    }
    if (page_count > MALLOC_MAX_PAGE) {
        return -1;
    }
    int err = alloc_phy_mem_page(phy_addr_alloc, page_count, paddrs);
    if (err) {
        return -1;
    }
    alloc_mem_use(vir_addr_alloc, vaddr, page_count, 1);
    return map_mem_page(page_dir, vaddr, paddrs, page_count, 1);
}

int unmalloc_mem(uint32_t page_dir, vir_addr_alloc_t *vir_addr_alloc,
                 uint32_t vaddr, int page_count) {
    int err = unalloc_mem(page_dir, vir_addr_alloc, vaddr, page_count);
    if (err) {
        return err;
    }
    // kernel memory dose not need to unmap
    if (page_dir != (uint32_t)kernel_page_dir) {
        err = unmap_mem(page_dir, vaddr, page_count);
    }
    return err;
}

void init_user_vir_addr_alloc(vir_addr_alloc_t *alloc) {
    uint32_t bytes_len = (user_all_memory / MEM_PAGE_SIZE + 7) / 8;
    uint32_t bitmap_pg_cnt = up2(bytes_len, MEM_PAGE_SIZE) / MEM_PAGE_SIZE;
    uint32_t bits = alloc_kernel_mem(bitmap_pg_cnt);
    init_addr_alloc(alloc, (uint8_t *)bits, kernel_all_memory, user_all_memory,
                    MEM_PAGE_SIZE);
}

bool count_mem_used_visitor(bitmap_t *btm, uint32_t idx_bit, void *arg) {
    uint32_t *count = (uint32_t *)arg;
    if (bitmap_scan_test(btm, idx_bit)) {
        *count = *count + 1;
    }
    return true;
}
uint32_t count_mem_used(vir_addr_alloc_t *alloc) {
    uint32_t count = 0;
    bitmap_foreach(&alloc->bitmap, 1, count_mem_used_visitor, &count);
    return count;
}
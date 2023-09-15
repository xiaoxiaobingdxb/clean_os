#ifndef CPU_MMU_H
#define CPU_MMU_H

#include "../types/basic.h"

// virtual address high 10 bit, is the index in pde array
#define pde_index(addr) (addr >> 22)
// virtual address middle 10 bit, is the index in pte array
#define pte_index(addr) ((addr >> 12) & 0x3ff)
// pde or pte value high 10 bit convert into physical address
#define pe_phy_addr(phy_page_value) (phy_page_value << 12)
// get offset in page from vaddr, add it on page_base is phy_addr
#define phy_addr(page_base, vaddr) (page_base + (0xfff & vaddr))

#pragma pack(1)  // aligned 1 bit
typedef union _pde_t {
    uint32_t v;
    struct {
        uint32_t present : 1;
        uint32_t write_disable : 1;
        uint32_t user_mode_acc : 1;
        uint32_t : 2;
        uint32_t accessd : 1;
        uint32_t : 1;
        uint32_t ps : 1;
        uint32_t : 4;
        uint32_t phy_pt_addr : 20;
    };
} pde_t;

typedef union _pte_t {
    uint32_t v;
    struct {
        uint32_t present : 1;
        uint32_t write_disable : 1;
        uint32_t user_mode_ac : 1;
        uint32_t : 2;
        uint32_t accessed : 1;
        uint32_t dirty : 1;
        uint32_t pat : 1;
        uint32_t global : 1;
        uint32_t : 3;
        uint32_t phy_page_addr : 20;
    };
} pte_t;
#pragma pack()

#endif
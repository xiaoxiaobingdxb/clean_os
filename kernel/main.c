#include "common/cpu/contrl.h"
#include "common/cpu/gdt.h"
#include "common/cpu/mem_page.h"
#include "common/tool/lib.h"

void test_invlpg() {
    pde_t *page_dir = (pde_t *)read_cr3();
    ASSERT(page_dir != (pde_t *)0);
    pte_t *pte = find_pte(page_dir, 0x80001000, 0);
    ASSERT(pte != (pte_t *)0);
    uint32_t phy_addr;
    int err = get_phy_addr(0x80001000, &phy_addr, 0);
    ASSERT(err == 0);
    pte = find_pte(page_dir, 0x80000000, 0);
    pte->v = phy_addr | (1 << 0) | (1 << 1) | (1 << 2);

    uint32_t *p = (uint32_t *)0x80000000;
    uint32_t value = *p;

    __asm__ __volatile__("invlpg (%[v])" ::[v] "r"(0x80000000));
    value = *p;
}

void test_paging() {
    uint32_t a = 10;
    uint32_t *p = &a;
    set_page();
    uint32_t vaddr = 0x80000000;
    p = (uint32_t *)0x80000000;
    uint32_t value = *p;
    int err = set_byte(vaddr, 0x12, 0);
    ASSERT(err == 0);
    value = *p;

    err = set_byte(0x80001000, 0x13, 1);
    ASSERT(err == 0);
    p = (uint32_t *)0x80001000;
    value = *p;
}
void main() {
    test_paging();
    test_invlpg();
    for (;;) {
    }
}
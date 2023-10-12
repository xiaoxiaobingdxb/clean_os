#include "common/cpu/contrl.h"
#include "common/cpu/gdt.h"
#include "common/cpu/mem_page.h"
#include "common/tool/lib.h"
#include "memory/config.h"
#include "memory/mem.h"
#include "thread/thread.h"
#include "common/lib/string.h"

void test_remap() {
    pde_t *page_dir = (pde_t *)read_cr3();
    ASSERT(page_dir != (pde_t *)0);
    pte_t *pte = find_pte(page_dir, 0x40001000, 0);
    ASSERT(pte != (pte_t *)0);
    uint32_t phy_addr;
    int err = get_phy_addr(0x40001000, &phy_addr, 0);
    ASSERT(err == 0);
    pte = find_pte(page_dir, 0x40000000, 0);
    pte->v = phy_addr | (1 << 0) | (1 << 1) | (1 << 2);
}
void test_invlpg() {
    test_remap();
    uint32_t *p = (uint32_t *)0x40000000;
    uint32_t value = *p;

    __asm__ __volatile__("invlpg (%[v])" ::[v] "r"(0x40000000));
    value = *p;
}

void test_reload_paing() {
    test_remap();

    uint32_t *p = (uint32_t *)0x40000000;
    uint32_t value = *p;

    pde_t *page_dir = (pde_t *)read_cr3();
    ASSERT(page_dir != (pde_t *)0);
    write_cr3((uint32_t)page_dir);
    value = *p;
}

void test_paging() {
    uint32_t a = 10;
    uint32_t *p = &a;
    set_page();
    uint32_t vaddr = 0x40000000;
    p = (uint32_t *)0x40000000;
    uint32_t value = *p;
    int err = set_byte(vaddr, 0x12, 0);
    ASSERT(err == 0);
    value = *p;

    err = set_byte(0x40001000, 0x13, 1);
    ASSERT(err == 0);
    p = (uint32_t *)0x40001000;
    value = *p;
}

void test_uv_page_dir() {
    uint32_t old_page_dir = current_page_dir();
    uint32_t user_vaddr = GB(1);
    uint32_t kernel_vaddr = GB(1) - KB(4);
    alloc_vm_for_page_dir(old_page_dir, user_vaddr, 4, PTE_W | PTE_U);
    alloc_vm_for_page_dir(old_page_dir, kernel_vaddr, 4, PTE_W | PTE_U);
    uint32_t *num_p = (uint32_t *)user_vaddr;
    (*num_p) = 0x1;
    uint32_t num = *num_p;
    ASSERT(num == 0x1);
    uint32_t *kernel_num_p = (uint32_t *)kernel_vaddr;
    (*kernel_num_p) = 0x2;
    num = *kernel_num_p;
    ASSERT(num = 0x2);

    uint32_t page_dir1 = create_uvm();
    uint32_t page_dir2 = create_uvm();
    set_page_dir(page_dir1);
    alloc_vm_for_page_dir(page_dir1, user_vaddr, 4, PTE_W | PTE_U);
    (*num_p) = 0x3;
    num = *num_p;
    ASSERT(num == 0x3);
    (*kernel_num_p) = 0x4;
    num = *kernel_num_p;
    ASSERT(num == 0x4);

    set_page_dir(page_dir2);
    alloc_vm_for_page_dir(page_dir2, user_vaddr, 4, PTE_W | PTE_U);
    (*num_p) = 0x5;
    num = *num_p;
    ASSERT(num == 0x5);
    (*kernel_num_p) = 0x6;
    num = *kernel_num_p;
    ASSERT(num == 0x6);

    set_page_dir(page_dir1);
    num = *num_p;
    ASSERT(num == 0x3);
    num = *kernel_num_p;
    ASSERT(num == 0x6);

    set_page_dir(old_page_dir);
    num = *num_p;
    ASSERT(num == 0x1);
    num = *kernel_num_p;
    ASSERT(num == 0x6);

    destroy_uvm(page_dir1);
    destroy_uvm(page_dir2);
    free_vm_for_page_dir(old_page_dir, user_vaddr, 4);
}

static struct boot_params *bparams;
void kernel_init(struct boot_params *params) {
    bparams = params;
    // test_paging();
    // test_invlpg();
    // test_reload_paing();

    init_mem(bparams);
}

task_struct *test1, *test2;

void test_thread_func1(void *arg) {
    char* name = (char*)arg;
    for (; ;) {
        switch_to(test1, test2);
    }
}
void test_thread_func2(void *arg) {
    char* name = (char*)arg;
    for (; ;) {
        switch_to(test2, test1);
    }
}

void main() {
    // test_uv_page_dir();
    test2 = thread_start("thread_test2", test_thread_func2, "thread_test2");
    test1 = thread_start1("thread_test1", test_thread_func1, "thread_test1");
    for (;;) {
        hlt();
    }
}
#ifndef INT_MEM_INFO_H
#define INT_MEM_INFO_H
#include "../types/basic.h"
#include "common/types/byte.h"

#define E820_MEM_TYPE_USABLE 1
#define E820_MEM_TYPE_RESERVED 2

struct boot_e820_entry
{
    uint32_t low_addr;
    uint32_t high_addr;
    uint32_t low_size;
    uint32_t high_size;
    uint32_t type;
};
struct boot_params
{
    // 15_e820
    struct boot_e820_entry e820_table[128];
    int e820_entry_count;

    // 15_e801
    uint32_t e801_k;

    // 15_88
    uint16_t ext_88_k;

};
extern struct boot_params boot_params;

void detect_memory(void);

#endif
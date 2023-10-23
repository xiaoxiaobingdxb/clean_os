#ifndef MEMORY_CONFIG_H
#define MEMORY_CONFIG_H

#include "common/types/byte.h"

#define GDT_TABLE_SIZE 256
#define KERNEL_SELECTOR_CS ((1 << 3) | 0) 
#define KERNEL_SELECTOR_DS ((2 << 3) | 0)
#define KERNEL_SELECTOR_SS KERNEL_SELECTOR_DS
#define USER_SELECTOR_CS ((3 << 3) | 3)
#define USER_SELECTOR_DS ((4 << 3) | 3)
#define USER_SELECTOR_SS USER_SELECTOR_DS
#define TSS_SELECTOR ((5 << 3) | 0)
#define KERNEL_STACK_SIZE KB(8)

#define KERNEL_MEM_SIZE_RATE 4

#endif
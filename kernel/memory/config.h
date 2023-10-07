#ifndef MEMORY_CONFIG_H
#define MEMORY_CONFIG_H

#include "common/types/byte.h"

#define GDT_TABLE_SIZE 256
#define KERNEL_SELECTOR_CS (1 * 8)
#define KERNEL_SELECTOR_DS (2 * 8)
#define KERNEL_STACK_SIZE KB(8)

#define KERNEL_MEM_SIZE_RATE 4

#endif
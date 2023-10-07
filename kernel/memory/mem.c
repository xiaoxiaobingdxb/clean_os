#include "mem.h"
#include "common/lib/string.h"
void init_mem(struct boot_params *params) {
    init_gdt();
    uint64_t all_mem = 0;
    if (params == NULL) {
        return;
    }
    struct boot_e820_entry *entity = params->e820_table;
    for (int i = 0; i < params->e820_entry_count; i++) {
        if (entity->type == E820_MEM_TYPE_USABLE ||
            entity->type == E820_MEM_TYPE_RESERVED) {
            uint64_t size = entity->low_addr;
            size += entity->low_size;
            if (size > all_mem) {
                all_mem = size;
            }
        }
        entity++;
    }
    init_paging(all_mem);
}
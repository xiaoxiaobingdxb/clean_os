#include "common/types/basic.h"
#include "malloc.h"
typedef struct {
    void* (*malloc)(size_t);
    void (*free)(void*);
} mallocator_t;

#define kernel_mallocator                                             \
    ({                                                                \
        mallocator_t mallocator = {.malloc = vmalloc, .free = vfree}; \
        mallocator;                                                   \
    })

#define user_mallocator                                             \
    ({                                                              \
        mallocator_t mallocator = {.malloc = malloc, .free = free}; \
        mallocator;                                                 \
    })

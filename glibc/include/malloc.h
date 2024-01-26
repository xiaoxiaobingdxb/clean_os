#if !defined(MALLOC_H)
#define MALLOC_H

#include "common/types/basic.h"

extern void *malloc(size_t size);
extern void free(void *ptr);
extern void *valloc(size_t size);
extern void *vmalloc(size_t size);
extern void vfree(void *ptr);

typedef struct {
    void *(*malloc)(size_t);
    void (*free)(void *);
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

#endif  // MALLOC_H

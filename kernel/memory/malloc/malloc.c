#include "./malloc.h"

#include "../../syscall/syscall_user.h"
#include "../../thread/thread.h"
#include "common/lib/list.h"
#include "common/lib/string.h"
#include "common/tool/math.h"

#define MEM_BLOCK_CNT 8  // from 2^3=8bit(byte) to 2^10=1024bit(1K)

typedef struct {
    size_t block_size;
    list free_blocks;
    int blocks_per_arena;
} mem_block_desc;

typedef struct {
    list_node mem_ele;
} mem_block;

typedef struct {
    mem_block_desc *desc;
    uint32_t free_cnt;
    bool large;
} mem_arena;

#define arena2block(arena, block_idx)                   \
    (mem_block *)((uint32_t)arena + sizeof(mem_arena) + \
                  block_idx * arena->desc->block_size)
#define block2arena(block) (mem_arena *)((uint32_t)block & 0xfffff000)

void init_malloc_desc(mem_block_desc *descs) {
    size_t block_size = 1 << 3;  // malloc min 1 byte
    for (int i = 0; i < MEM_BLOCK_CNT; i++) {
        descs[i].block_size = block_size;
        descs[i].blocks_per_arena =
            (MEM_PAGE_SIZE - sizeof(mem_arena)) / block_size;
        init_list(&descs[i].free_blocks);
        block_size *= 2;
    }
}

typedef struct {
    mem_block_desc descs[MEM_BLOCK_CNT];
    bool init;
    void *(*allocator)(size_t);
    void (*unallocator)(void *, size_t);
} malloc_state;

malloc_state *main_state = NULL;
malloc_state *kernel_state = NULL;

void *_alloc_kernel_mem(size_t size);
void _unalloc_kernel_mem(void *ptr, size_t size);

void _init_malloc(malloc_state *state) {
    if (state->init) {
        return;
    }
    state->init = true;
    init_malloc_desc(state->descs);
}

void init_malloc(bool isKernel) {
    malloc_state *state;
    if (isKernel) {
        if (!kernel_state) {
            kernel_state =
                (malloc_state *)_alloc_kernel_mem(sizeof(malloc_state));
            memset(kernel_state, 0, sizeof(malloc_state));
            kernel_state->init = false;
            kernel_state->allocator = _alloc_kernel_mem;
            kernel_state->unallocator = _unalloc_kernel_mem;
        }
        state = kernel_state;
    } else {
        if (!main_state) {
            main_state = (malloc_state *)mmap_anonymous(sizeof(malloc_state));
            memset(main_state, 0, sizeof(malloc_state));
            main_state->init = false;
            main_state->allocator = mmap_anonymous;
            main_state->unallocator = munmap;
        }
        state = main_state;
    }
    _init_malloc(state);
}

void *_malloc(size_t size, bool isKernel) {
    init_malloc(isKernel);
    malloc_state *state = main_state;
    if (isKernel) {
        state = kernel_state;
    }
    if (size > state->descs[MEM_BLOCK_CNT - 1].block_size) {
        size += sizeof(mem_arena);
        size = up2(size, MEM_PAGE_SIZE);
        uint32_t paeg_count = size / MEM_PAGE_SIZE;
        mem_arena *a = (mem_arena *)state->allocator(size);
        memset(a, 0, size);
        a->desc == NULL;
        a->free_cnt = paeg_count;
        a->large = true;
        return (void *)((uint32_t)a + sizeof(mem_arena));
    }
    int idx;
    for (idx = 0; idx < MEM_BLOCK_CNT; idx++) {
        if (size <= state->descs[idx].block_size) {
            break;
        }
    }
    mem_block *b;
    if (state->descs[idx].free_blocks.size == 0) {
        mem_arena *arena = (mem_arena *)state->allocator(MEM_PAGE_SIZE);
        memset(arena, 0, sizeof(arena));
        arena->desc = &state->descs[idx];
        arena->free_cnt = state->descs[idx].blocks_per_arena;
        arena->large = false;
        for (int i = 0; i < arena->free_cnt; i++) {
            b = arena2block(arena, i);
            pushr(&arena->desc->free_blocks, &b->mem_ele);
        }
    }
    list_node *b_node = popl(&state->descs[idx].free_blocks);
    b = tag2entry(mem_block, mem_ele, b_node);
    memset(b, 0, state->descs[idx].block_size);
    mem_arena *arean = block2arena(b);
    arean->free_cnt--;
    return (void *)b;
}
void *malloc(size_t size) { return _malloc(size, false); }

void _free(void *ptr, void (*unallocator)(void *, size_t)) {
    if (ptr == NULL) {
        return;
    }
    // move this block to free_blocks
    mem_block *block = (mem_block *)ptr;
    mem_arena *arena = block2arena(block);
    if (arena->large) {
        munmap((void *)arena, arena->free_cnt * MEM_PAGE_SIZE);
        return;
    }
    pushr(&arena->desc->free_blocks, &block->mem_ele);
    arena->free_cnt++;

    // release free_block to os
    if (arena->desc->free_blocks.size >= arena->desc->blocks_per_arena * 2 &&
        arena->free_cnt == arena->desc->blocks_per_arena) {
        for (int i = 0; i < arena->free_cnt; i++) {
            mem_block *b = arena2block(arena, i);
            remove(&arena->desc->free_blocks, &b->mem_ele);
        }
        unallocator((void *)arena, MEM_PAGE_SIZE);
    }
}

void free(void *ptr) { _free(ptr, munmap); }

void *_alloc_kernel_mem(size_t size) {
    int page_count = up2(size, MEM_PAGE_SIZE) / MEM_PAGE_SIZE;
    return (void *)alloc_kernel_mem(page_count);
}

void *vmalloc(size_t size) { return _malloc(size, true); }

void _unalloc_kernel_mem(void *ptr, size_t size) {
    int page_count = up2(size, MEM_PAGE_SIZE) / MEM_PAGE_SIZE;
    unalloc_kernel_mem((uint32_t)ptr, page_count);
}

void vfree(void *ptr) { _free(ptr, _unalloc_kernel_mem); }
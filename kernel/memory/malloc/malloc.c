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
} malloc_state;

static malloc_state main_state;
static bool __malloc_init;

void init_malloc() {
    if (__malloc_init) {
        return;
    }
    __malloc_init = true;
    init_malloc_desc(main_state.descs);
}
void *_malloc(size_t size, void*(*allocator)(size_t)) {
    init_malloc();
    if (size > main_state.descs[MEM_BLOCK_CNT - 1].block_size) {
        size = up2(size, MEM_PAGE_SIZE);
        uint32_t paeg_count =  size / MEM_PAGE_SIZE;
        mem_arena *a = (mem_arena*)allocator(size);
        memset(a, 0, size);
        a->desc == NULL;
        a->free_cnt = paeg_count;
        a->large = true;
        return (void*)((uint32_t)a + sizeof(mem_arena));
    }
    int idx;
    for (idx = 0; idx < MEM_BLOCK_CNT; idx++) {
        if (size <= main_state.descs[idx].block_size) {
            break;
        }
    }
    mem_block *b;
    if (main_state.descs[idx].free_blocks.size == 0) {
        mem_arena *arena = (mem_arena *)allocator(MEM_PAGE_SIZE);
        memset(arena, 0, sizeof(arena));
        arena->desc = &main_state.descs[idx];
        arena->free_cnt = main_state.descs[idx].blocks_per_arena;
        arena->large = false;
        for (int i = 0; i < arena->free_cnt; i++) {
            b = arena2block(arena, i);
            pushr(&arena->desc->free_blocks, &b->mem_ele);
        }
    }
    list_node *b_node = popl(&main_state.descs[idx].free_blocks);
    b = tag2entry(mem_block, mem_ele, b_node);
    memset(b, 0, main_state.descs[idx].block_size);
    mem_arena *arean = block2arena(b);
    arean->free_cnt--;
    return (void *)b;
}
void *malloc(size_t size) {
    return _malloc(size, mmap_anonymous);
}

void _free(void *ptr, void(*unallocator)(void*, size_t)) {
    // move this block to free_blocks
    mem_block *block = (mem_block *)ptr;
    mem_arena *arena = block2arena(block);
    if (arena->large) {
        munmap((void*)arena, arena->free_cnt * MEM_PAGE_SIZE);
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
        unallocator((void*)arena, MEM_PAGE_SIZE);
    }
}

void free(void *ptr) {
    _free(ptr, munmap);
}

void* _alloc_kernel_mem(size_t size) {
    int page_count = up2(size, MEM_PAGE_SIZE) / MEM_PAGE_SIZE;
    return (void*)alloc_kernel_mem(page_count);
}

void *vmalloc(size_t size) {
    return _malloc(size, _alloc_kernel_mem);
}

void _unalloc_kernel_mem(void *ptr, size_t size) {
    int page_count = up2(size, MEM_PAGE_SIZE) / MEM_PAGE_SIZE;
    unalloc_kernel_mem((uint32_t)ptr, page_count);
}

void vfree(void *ptr) {
    _free(ptr, _unalloc_kernel_mem);
}
#include "mblock.h"
#include "common/types/basic.h"

/**
 * @brief 初始化存储块管理器
 * 将mem开始的内存区域划分成多个相同大小的内存块，然后用链表链接起来
 */
net_err_t mblock_init (mblock_t* mblock, void * mem, int blk_size, int cnt) {

    // 将缓存区分割成一块块固定大小内存，插入到队列中
    uint8_t* buf = (uint8_t*)mem;
    nlist_init(&mblock->free_list);
    for (int i = 0; i < cnt; i++, buf += blk_size) {
        nlist_node_t* block = (nlist_node_t*)buf;
        nlist_node_init(block);
        nlist_insert_last(&mblock->free_list, block);
    }


    return NET_ERR_OK;
}

/**
 * 分配一个空闲的存储块
 */
void * mblock_alloc(mblock_t* mblock, int ms) {
    // 无需等待的分配，查询后直接退出
    if (ms < 0) {
        int count = nlist_count(&mblock->free_list);
        // 没有，则直接返回了，无等待则直接退出
        if (count == 0) {
            return (void*)0;
        }
    }


    // 获取分配得到的项
    nlist_node_t* block = nlist_remove_first(&mblock->free_list);
    return block;
}


/**
 * 获取空闲块数量
 */
int mblock_free_cnt(mblock_t* mblock) {
    int count = nlist_count(&mblock->free_list);
    return count;
}

/**
 * 释放存储块
 */
void mblock_free(mblock_t* mblock, void* block) { 
    nlist_insert_last(&mblock->free_list, (nlist_node_t *)block);
}

/**
 * 销毁存储管理块
 */
void mblock_destroy(mblock_t* mblock) {
}

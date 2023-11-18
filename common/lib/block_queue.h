#if !defined(LIB_BLOCK_QUEUE_H)
#define LIB_BLOCK_QUEUE_H

#include "../types/basic.h"
#include "../types/std.h"

#define queue_buf_size 1024
typedef void(*block_fun)(int*);
typedef struct {
    int producer, consumer;
    block_fun wait, wakeup;
    void* buf[queue_buf_size];
    int head, tail;
} block_queue;

void init_queue(block_queue *queue, block_fun wait_fun, block_fun wakeup_fun);
bool queue_full(block_queue*);
size_t queue_len(block_queue*);
void* queue_get(block_queue*);
void queue_put(block_queue*, void*);

#endif  // LIB_BLOCK_QUEUE_H

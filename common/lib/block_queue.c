#include "block_queue.h"
void init_queue(block_queue *queue, block_fun wait_fun, block_fun wakeup_fun) {
    queue->wait = wait_fun;
    queue->wakeup = wakeup_fun;
    queue->consumer = queue->producer = 0;
    queue->head = queue->tail = 0;
}
int next_pos(int pos) {
    return (pos + 1) % queue_buf_size;
}

bool queue_full(block_queue* queue) {
    return next_pos(queue->head) == queue->tail;
}
bool queue_empty(block_queue *queue) {
    return queue->head == queue->tail;
}
size_t queue_len(block_queue* queue) {
    if (queue->head >= queue->tail) {
        return queue->head - queue->tail;
    }
    return queue_buf_size - (queue->tail - queue->head);
}
void* queue_get(block_queue* queue) {
    while (queue_empty(queue) && queue->wait != NULL)
    {
        queue->wait(&queue->consumer);
    }
    if (queue_empty(queue)) {
        return NULL;
    }
    void *value = queue->buf[queue->tail];
    queue->tail = next_pos(queue->tail);
    if (queue->producer != 0 && queue->wakeup != NULL) {
        queue->wakeup(&queue->producer);
        queue->producer = 0;
    }
    return value;
}
void queue_put(block_queue* queue, void *vlaue) {
    while (queue_full(queue) && queue->wait != NULL)
    {
        queue->wait(&queue->producer);
    }
    queue->buf[queue->head] = vlaue;
    queue->head = next_pos(queue->head);
    if (queue->consumer != 0 && queue->wakeup != NULL) {
        queue->wakeup(&queue->consumer);
        queue->consumer = 0;
    }
}
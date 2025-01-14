/**
 * @file fixq.c
 * @brief 定长消息队列
 * @brief
 * @version 0.1
 * @date 2022-10-25
 *
 * @copyright Copyright (c) 2022
 *
 * 在整个协议栈中，数据包需要排队，线程之间的通信消息也需要排队，因此需要
 * 借助于消息队列实现。该消息队列长度是定长的，以避免消息数量太多耗费太多资源
 */
#include "fixq.h"

/**
 * @brief 初始化定长消息队列
 */
net_err_t fixq_init(fixq_t *q, void **buf, int size) {
    q->size = size;
    q->in = q->out = q->cnt = 0;
    q->buf = (void **)0;
    q->buf = buf;

    init_segment(&q->send_sem, size);
    init_segment(&q->receive_sem, 0);
    return NET_ERR_OK;
}

/**
 * @brief 向消息队列写入一个消息
 * 如果消息队列满，则看下tmo，如果tmo < 0则不等待
 */
net_err_t fixq_send(fixq_t *q, void *msg, int tmo) {
    if ((q->cnt >= q->size) && (tmo < 0)) {
        // 如果缓存已满，并且不需要等待，则立即退出
        return NET_ERR_FULL;
    }

    segment_wait(&q->send_sem, NULL);

    // 有空闲单元写入缓存
    q->buf[q->in++] = msg;
    if (q->in >= q->size) {
        q->in = 0;
    }
    q->cnt++;

    segment_wakeup(&q->receive_sem, NULL);
    return NET_ERR_OK;
}

/**
 * @brief 从数据包队列中取一个消息，如果无，则等待
 */
void *fixq_recv(fixq_t *q, int tmo) {
    if (!q->cnt && (tmo < 0)) {
        return (void *)0;
    }
    
    segment_wait(&q->receive_sem, NULL);

    // 取消息
    void *msg = q->buf[q->out++];
    if (q->out >= q->size) {
        q->out = 0;
    }
    q->cnt--;

    segment_wakeup(&q->send_sem, NULL);
    return msg;
}

/**
 * 销毁队列
 * @param list 待销毁的队列
 */
void fixq_destroy(fixq_t *q) {
}
/**
 * @brief 取缓冲中消息的数量
 */
int fixq_count (fixq_t *q) {
    int count = q->cnt;
    return count;
}

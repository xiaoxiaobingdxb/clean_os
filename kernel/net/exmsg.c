/**
 * @brief TCP/IP核心线程通信模块
 * 此处运行了一个核心线程，所有TCP/IP中相关的事件都交由该线程处理
 * @author lishutong (527676163@qq.com)
 * @version 0.1
 * @date 2022-08-19
 * @copyright Copyright (c) 2022
 * @note
 */
#include "exmsg.h"
#include "fixq.h"
#include "mblock.h"
#include "common/tool/log.h"
#define EXMSG_MSG_CNT          10                 // 消息缓冲区大小
static void * msg_tbl[EXMSG_MSG_CNT];  // 消息缓冲区
static fixq_t msg_queue;            // 消息队列
static exmsg_t msg_buffer[EXMSG_MSG_CNT];  // 消息块
static mblock_t msg_block;          // 消息分配器

/**
 * @brief 收到来自网卡的消息
 */
net_err_t exmsg_netif_in(netif_t* netif) {
    // 分配一个消息结构
    exmsg_t* msg = mblock_alloc(&msg_block, -1);
    if (!msg) {
        return NET_ERR_MEM;
    }

    // 写消息内容
    msg->type = NET_EXMSG_NETIF_IN;
    msg->netif.netif = netif;

    // 发送消息
    net_err_t err = fixq_send(&msg_queue, msg, -1);
    if (err < 0) {
        mblock_free(&msg_block, msg);
        return err;
    }

    return NET_ERR_OK;
}


/**
 * @brief 执行内部工作函数调用
 */
net_err_t exmsg_func_exec(exmsg_func_t func, void * param) {
    // 构造消息
    func_msg_t func_msg;
    func_msg.func = func;
    func_msg.param = param;
    func_msg.err = NET_ERR_OK;

    // 分配消息结构
    exmsg_t* msg = (exmsg_t*)mblock_alloc(&msg_block, 0);
    msg->type = NET_EXMSG_FUN; 
    msg->func = &func_msg;

    // 发消息给工作线程去执行
    net_err_t err = fixq_send(&msg_queue, msg, 0);
    if (err < 0) {
        mblock_free(&msg_block, msg);
        return err;
    }

    return func_msg.err;
}

/**
 * @brief 执行工作函数
 */
static net_err_t do_func(func_msg_t* func_msg) {
    
    func_msg->err = func_msg->func(func_msg);

    return NET_ERR_OK;
}

/**
 * @brief 核心线程通信初始化
 */
net_err_t exmsg_init(void) {

    // 初始化消息队列
    net_err_t err = fixq_init(&msg_queue, msg_tbl, EXMSG_MSG_CNT);
    if (err < 0) {
        return err;
    }

    // 初始化消息分配器
    err = mblock_init(&msg_block, msg_buffer, sizeof(exmsg_t), EXMSG_MSG_CNT);
    if (err < 0) {
        return err;
    }

    // 初始化完成
    return NET_ERR_OK;
}

net_err_t do_netif_in(exmsg_t *msg) {
    netif_t *netif = msg->netif.netif;
    pktbuf_t *buf;
    while( (buf = netif_get_in(netif, -1))) {
        log_debug("receive a packet\n");
        net_err_t err;
        if (netif->link_layer) {
            err = netif->link_layer->in(netif, buf);
            if (err < 0) {
                pktbuf_free(buf);
                log_warn("netif_in failed, err=%d\n", err);
            }
        } else {
            err = ipv4_in(netif, buf);
            if (err < 0) {
                pktbuf_free(buf);
            }
            log_err("no link_layer");
        }
    }
    return NET_ERR_OK;
}

void net_work_thread(void *args) {
    log_debug("exmsg running...\n");
    while (1) {
        exmsg_t *msg = (exmsg_t*)fixq_recv(&msg_queue, 0);
        if (msg) {
            log_debug("receive msg(%d)\n", msg->type);
            switch (msg->type)
            {
            case NET_EXMSG_NETIF_IN:
                do_netif_in(msg);
                break;
            default:
                break;
            }
            mblock_free(&msg_block, msg);
        }
    }
    
}
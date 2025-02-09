﻿#ifndef NET_EXMSG_H
#define NET_EXMSG_H

#include "net_err.h"
#include "nlist.h"
#include "netif.h"
#include "pktbuf.h"
#include "../thread/thread.h"

struct _func_msg_t;
typedef net_err_t(*exmsg_func_t)(struct _func_msg_t* msg);
struct _req_msg_t;

/**
 * 工作函数调用
 */
typedef struct _func_msg_t {
    uint32_t thread_page_dir;        // 当前执行调用的线程

    exmsg_func_t func;         // 被调用函数
    void* param;            // 参数
    net_err_t err;          // 错误码

    segment_t wait_sem;     // 等待信号量
}func_msg_t;

/**
 * 网络接口消息
 */
typedef struct _msg_netif_t {
    netif_t* netif;                 // 消息的来源
}msg_netif_t;

/**
 * @brief 传递给核心线程的消息
 */
typedef struct _exmsg_t {
    // 消息类型
    enum {
        NET_EXMSG_NETIF_IN,             // 网络接口数据消息
        NET_EXMSG_FUN,                  // 工作函数调用
    }type;

    // 消息数据
    union {
        msg_netif_t netif;              // 网络接口的消息
        func_msg_t* func;               // 工作函数调用消息
    };
}exmsg_t;

net_err_t exmsg_init(void);
net_err_t exmsg_start(void);

net_err_t exmsg_netif_in(netif_t* netif);
net_err_t exmsg_func_exec(exmsg_func_t func, void* param);

#endif // NET_EXMSG_H
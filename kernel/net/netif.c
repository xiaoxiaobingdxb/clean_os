#include "netif.h"

#include "common/lib/string.h"
#include "ip.h"
#include "exmsg.h"
// #include "exmsg.h"

const link_layer_t* link_layers[NETIF_TYPE_SIZE];
net_err_t netif_register_layer(netif_type_t type, const link_layer_t* layer) {
    if (type < 0 || type >= NETIF_TYPE_SIZE) {
        return NET_ERR_PARAM;
    }
    if (link_layers[type]) {
        return NET_ERR_EXIST;
    }
    link_layers[type] = layer;
}

const link_layer_t* netif_get_layer(netif_type_t type) {
    if (type < 0 || type >= NETIF_TYPE_SIZE) {
        return (const link_layer_t*)NULL;
    }
    return link_layers[type];
}

#define netif_pool_size 5
netif_t netif_pool[netif_pool_size];

list netif_list;
void init_netif() { init_list(&netif_list); }

netif_t* netif_open(const char* dev_name, const netif_ops_t* ops,
                    void* ops_data) {
    netif_t* netif;
    for (int i = 0; i < netif_pool_size; i++) {
        netif_t* ni = &netif_pool[i];
        if (ni->state == NETIF_CLOSED) {
            netif = ni;
            break;
        }
    }
    if (netif == NULL) {
        return (netif_t*)NULL;
    }
    netif->ip_desc.ip_addr.q_addr = 0;
    netif->ip_desc.net_mask.q_addr = 0;
    netif->ip_desc.gateway.q_addr = 0;

    netif->type = NETIF_TYPE_NONE;
    strcpy(netif->name, dev_name);
    netif->ops = ops;
    netif->ops_data = ops_data;

    net_err_t err = fixq_init(&netif->in_q, netif->in_q_buf, NETIF_INQ_SIZE);
    if (err < 0) {
        return (netif_t*)NULL;
    }
    err = fixq_init(&netif->out_q, netif->out_q_buf, NETIF_OUTQ_SIZE);
    if (err < 0) {
        return (netif_t*)NULL;
    }

    err = netif->ops->open(netif, ops_data);
    if (err != NET_ERR_OK) {
        goto free_return;
    }
    netif->state = NETIF_OPENED;
    if (netif->type == NETIF_TYPE_NONE) {
        goto free_return;
    }
    netif->link_layer = netif_get_layer(netif->type);
    if (!netif->link_layer && netif->type != NETIF_TYPE_LOOP) {
        goto free_return;
    }
    pushr(&netif_list, &netif->node);
    return netif;
free_return:
    netif->ops->close(netif);
    netif->state = NETIF_CLOSED;
}
static netif_t* netif_default;
int ipaddr_is_any(const ip_addr_t* ip) { return ip->q_addr == 0; }
void netif_set_default(netif_t* netif) {
    // 添加新缺省路由, 仅当网关有效时
    ip_addr_t *any = ipaddr_get_any();
    ip_desc_t desc;
    desc.ip_addr.q_addr = any->q_addr;
    desc.net_mask.q_addr = any->q_addr;
    if (!ipaddr_is_any(&netif->ip_desc.gateway)) {
        // 如果已经设置了缺省的，移除缺省路由
        if (netif_default) {
            route_remove(&desc);
        }

        // 再添加新的路由
        route_add(&desc, &netif->ip_desc.gateway,
                  netif);
    }

    // 纪录新的网卡
    netif_default = netif;
}
net_err_t netif_set_active(netif_t* netif) {
    if (netif->state != NETIF_OPENED) {
        return NET_ERR_STATE;
    }
    if (netif->link_layer) {
        net_err_t err = netif->link_layer->open(netif);
        if (err < 0) {
            return err;
        }
    }
    ip_addr_t ip;
    ip.q_addr = netif->ip_desc.ip_addr.q_addr & netif->ip_desc.net_mask.q_addr;
    ip_desc_t desc;
    desc.ip_addr.q_addr = ip.q_addr;
    desc.net_mask.q_addr = netif->ip_desc.net_mask.q_addr;
    desc.gateway.q_addr = netif->ip_desc.gateway.q_addr;
    route_add(&desc, ipaddr_get_any(), netif);
    ip.q_addr = 0xFFFFFFFF;
    route_add(&desc, ipaddr_get_any(), netif);
    if (!netif_default && (netif->type != NETIF_TYPE_LOOP)) {
        netif_default = netif;
    }
    netif->state = NETIF_ACTIVE;
    return NET_ERR_OK;
}

/**
 * @brief 将buf加入到网络接口的输入队列中
 */
net_err_t netif_put_in(netif_t* netif, pktbuf_t* buf, int tmo) {
    // 写入接收队列
    net_err_t err = fixq_send(&netif->in_q, buf, tmo);
    if (err < 0) {
        return NET_ERR_FULL;
    }

    // 当输入队列中只有刚写入的这个包时，发消息通知工作线程去处理
    // 可能的情况：
    // 1. 数量为1，只有刚写入的包，发消息通知
    // 2. 数量为0，前面刚写入，正好立即被工作线程处理掉，无需发消息
    // 3. 数量超过1，即有累积的包，工作线程正在处理，无需发消息
    if (fixq_count(&netif->in_q) == 1) {
        exmsg_netif_in(netif);
    }
    return NET_ERR_OK;
}

/**
 * @brief 将Buf添加到网络接口的输出队列中
 */
net_err_t netif_put_out(netif_t* netif, pktbuf_t* buf, int tmo) {
    // 写入发送队列
    net_err_t err = fixq_send(&netif->out_q, buf, tmo);
    if (err < 0) {
        return err;
    }

    // 只是写入队列，具体的发送会调用ops->xmit来启动发送
    return NET_ERR_OK;
}

/**
 * @brief 从输入队列中取出一个数据包
 */
pktbuf_t* netif_get_in(netif_t* netif, int tmo) {
    // 从接收队列中取数据包
    pktbuf_t* buf = fixq_recv(&netif->in_q, tmo);
    if (buf) {
        // 重新定位，方便进行读写
        pktbuf_reset_acc(buf);
        return buf;
    }

    return (pktbuf_t*)0;
}

/**
 * 从输出队列中取出一个数据包
 */
pktbuf_t* netif_get_out(netif_t* netif, int tmo) {
    // 从发送队列中取数据包，不需要等待。可能会被中断处理程序中调用
    // 因此，不能因为没有包而挂起程序
    pktbuf_t* buf = fixq_recv(&netif->out_q, tmo);
    if (buf) {
        // 重新定位，方便进行读写
        pktbuf_reset_acc(buf);
        return buf;
    }

    return (pktbuf_t*)0;
}

/**
 * @brief 发送一个网络包到网络接口上, 目标地址为ipaddr
 * 在发送前，先判断驱动是否正在发送，如果是则将其插入到发送队列，等驱动有空后，由驱动自行取出发送。
 * 否则，加入发送队列后，启动驱动发送
 */
net_err_t netif_out(netif_t* netif, ip_addr_t* ipaddr, pktbuf_t* buf) {
    // 发往外部，根据不同的接口类型作不同处理
    if (netif->link_layer) {
        net_err_t err = netif->link_layer->out(netif, ipaddr, buf);
        if (err < 0) {
            return err;
        }
        return NET_ERR_OK;
    } else {
        // 缺省情况，将数据包插入就绪队列，然后通知驱动程序开始发送
        // 硬件当前发送如果未进行，则启动发送，否则不处理，等待硬件中断自动触发进行发送
        net_err_t err = netif_put_out(netif, buf, -1);
        if (err < 0) {
            return err;
        }

        // 启动发送
        return netif->ops->xmit(netif);
    }
}

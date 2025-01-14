#include "netif.h"
#include "util.h"
#include "common/lib/string.h"
#include "common/tool/log.h"
#include "ehter.h"
#include "pktbuf.h"
#include "arp.h"

#define ETH_DATA_MIN 46  // 最小发送的帧长，即头部+46
net_err_t ether_raw_out(netif_t* netif, uint16_t protocol, const uint8_t* dest,
                        pktbuf_t* buf) {
    net_err_t err;

    // 以太网最小帧长度有要求，这里进行调整，使得驱动就不用关心这个问题
    int size = pktbuf_total(buf);
    if (size < ETH_DATA_MIN) {
        // 调整至46字节
        err = pktbuf_resize(buf, ETH_DATA_MIN);
        if (err < 0) {
            return err;
        }

        // 填充新增的部分为0
        pktbuf_reset_acc(buf);
        pktbuf_seek(buf, size);
        pktbuf_fill(buf, 0, ETH_DATA_MIN - size);
    }

    // 调整读写位置，预留以太网包头，并设置为连续位置
    err = pktbuf_add_header(buf, sizeof(ether_hdr_t), 1);
    if (err < 0) {
        return NET_ERR_SIZE;
    }

    // 填充以太网帧头，发送
    ether_pkt_t* pkt = (ether_pkt_t*)pktbuf_data(buf);
    memcpy(pkt->hdr.dest, dest, ETH_HWA_SIZE);
    memcpy(pkt->hdr.src, netif->mac, ETH_HWA_SIZE);
    pkt->hdr.protocol = x_htons(protocol);  // 注意大小端转换

    // 如果是发给自己的，那么直接写入发送队列。
    // 例如，应用层可能ping本机某个网卡的IP，显然不能将这个发到网络上
    // 而是应当插入到发送队列中。可以考虑在IP模块中通过判断IP地址来处理，也可以像这样
    // 在底层处理。当然，这样处理的效率会低一些
    if (memcmp(netif->mac, dest, ETH_HWA_SIZE) == 0) {
        return netif_put_in(netif, buf, -1);
    } else {
        err = netif_put_out(netif, buf, -1);
        if (err < 0) {
            return err;
        }
        // 通知网卡开始发送
        return netif->ops->xmit(netif);
    }
}

static const uint8_t empty_hwaddr[] = {0, 0, 0, 0, 0, 0};  // 空闲硬件地址
const uint8_t* ether_broadcast_addr(void) {
    static const uint8_t broadcast_addr[] = {0xFF, 0xFF, 0xFF,
                                             0xFF, 0xFF, 0xFF};
    return broadcast_addr;
}
net_err_t arp_make_request(netif_t* netif, ip_addr_t* pro_addr) {
    // 分配ARP包的空间
    pktbuf_t* buf = pktbuf_alloc(sizeof(arp_pkt_t));
    if (buf == NULL) {
        return NET_ERR_NONE;
    }

    // 这里看起来似乎什么必要，不过还是加上吧
    pktbuf_set_cont(buf, sizeof(arp_pkt_t));

    // 填充ARP包，请求包，写入本地的ip和mac、目标Ip，目标硬件写空值
    arp_pkt_t* arp_packet = (arp_pkt_t*)pktbuf_data(buf);
    arp_packet->htype = x_htons(ARP_HW_ETHER);
    arp_packet->ptype = x_htons(NET_PROTOCOL_IPv4);
    arp_packet->hlen = 6;
    arp_packet->plen = 4;
    arp_packet->opcode = x_htons(ARP_REQUEST);
    memcpy(arp_packet->send_haddr, netif->mac, 6);
    ip_addr_to_buf(&netif->ip_desc.ip_addr, arp_packet->send_paddr);
    memcpy(arp_packet->target_haddr, empty_hwaddr, 6);
    ip_addr_to_buf(pro_addr, arp_packet->target_paddr);

    // 发广播通知所有主机
    net_err_t err =
        ether_raw_out(netif, NET_PROTOCOL_ARP, ether_broadcast_addr(), buf);
    if (err < 0) {
        pktbuf_free(buf);
    }
    return err;
}

static net_err_t ether_open(netif_t* netif) {
    return arp_make_request(netif, &netif->ip_desc.ip_addr);
}

static void ether_close(netif_t* netif) {
    // arp_clear(netif);
}

net_err_t pkt_buf_check(pktbuf_t* buf, int total_size) {
    if (total_size > (sizeof(ether_hdr_t) + ETH_MTU)) {
        return NET_ERR_SIZE;
    }
    if (total_size < sizeof(ether_hdr_t)) {
        return NET_ERR_SIZE;
    }
    return NET_ERR_OK;
}

static void show_ether_pkt(char* title, ether_pkt_t* pkt, int size) {
    ether_hdr_t* hdr = (ether_hdr_t*)pkt;
    log_debug("------------------%s------------------\n", title);
    log_debug("len:%d    ", size);
    char* mac = "ff:ff:ff:ff:ff:ff";
    mac2str(hdr->dest, mac);
    log_debug("dest:%s    ", mac);
    mac2str(hdr->src, mac);
    log_debug("src:%s    ", mac);
    log_debug("protocol:%d    ", x_htons(hdr->protocol));
    switch (x_htons(hdr->protocol)) {
    case NET_PROTOCOL_ARP:
        log_debug("arp");
        break;
    case NET_PROTOCOL_IPv4:
        log_debug("ip");
        break;
    }
    log_debug("\n");
}

static net_err_t ether_in(netif_t* netif, pktbuf_t* buf) {
    pktbuf_set_cont(buf, sizeof(ether_hdr_t));
    ether_pkt_t* pkt = (ether_pkt_t*)pktbuf_data(buf);
    net_err_t err = pkt_buf_check(buf, buf->total_size);
    if (err < 0) {
        return err;
    }
    show_ether_pkt("ether_in", pkt, buf->total_size);
    switch (x_htons(pkt->hdr.protocol)) {
    case NET_PROTOCOL_ARP: {
        err = pktbuf_remove_header(buf, sizeof(ether_hdr_t));
        if (err < 0) {
            return err;
        }
        return arp_in(netif, buf);
    }
    case NET_PROTOCOL_IPv4: {
        arp_update_from_ipbuf(netif, buf);
        break;
    }
    }
    return NET_ERR_OK;
}

static net_err_t ether_out(netif_t* netif, ip_addr_t* ip_addr, pktbuf_t* buf) {
    return NET_ERR_OK;
}

net_err_t init_ether() {
    static const link_layer_t link_layer = {
        .type = NETIF_TYPE_ETHER,
        .open = ether_open,
        .close = ether_close,
        .in = ether_in,
        .out = ether_out,
    };
    net_err_t err = netif_register_layer(NETIF_TYPE_ETHER, &link_layer);
    if (err < 0) {
        return err;
    }
    return NET_ERR_OK;
}
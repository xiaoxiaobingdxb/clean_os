#if !defined(NET_ETHER_H)
#define NET_ETHER_H
#include "net_err.h"
#include "common/types/basic.h"
#include "util.h"
#include "netif.h"
#include "protocol.h"

#pragma pack(1)
#define ETH_HWA_SIZE 6  // mac地址长度
#define ETH_MTU 1500    // 最大传输单元，数据区的大小
/**
 * @brief 以太网帧头
 */
typedef struct _ether_hdr_t {
    uint8_t dest[ETH_HWA_SIZE];  // 目标mac地址
    uint8_t src[ETH_HWA_SIZE];   // 源mac地址
    uint16_t protocol;           // 协议/长度
} ether_hdr_t;

/**
 * @brief 以太网帧格式
 * 肯定至少要求有1个字节的数据
 */
typedef struct _ether_pkt_t {
    ether_hdr_t hdr;        // 帧头
    uint8_t data[ETH_MTU];  // 数据区
} ether_pkt_t;

#pragma pack()

net_err_t init_ether();
net_err_t ether_raw_out(netif_t *netif, uint16_t protocol, const uint8_t *dest, pktbuf_t *buf);
net_err_t arp_make_request(netif_t *netif, ip_addr_t *pro_addr);


#endif // NET_ETHER_H

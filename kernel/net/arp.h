#if !defined(NET_ARP_H)
#define NET_ARP_H

#include "net_err.h"
#include "netif.h"
#include "pktbuf.h"
#include "ether.h"

// @ref https://wiki.osdev.org/Address_Resolution_Protocol
#pragma pack(1)
typedef struct {
    uint16_t htype;
    uint16_t ptype;
    uint8_t hlen;
    uint8_t plen;
    uint16_t opcode;
    uint8_t send_haddr[ETH_HWA_SIZE];
    uint8_t send_paddr[IPV4_ADDR_SIZE];
    uint8_t target_haddr[ETH_HWA_SIZE];
    uint8_t target_paddr[IPV4_ADDR_SIZE];
} arp_pkt_t;
#pragma pack()

void init_arp();
net_err_t arp_in(netif_t *netif, pktbuf_t *buf);
net_err_t arp_update_from_ipbuf(netif_t *netif, pktbuf_t *buf);
uint8_t* arp_find(ip_addr_t *ip);
#endif // NET_ARP_H

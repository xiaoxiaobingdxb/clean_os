#include "arp.h"
#include "ether.h"
#include "common/tool/log.h"
#include "mblock.h"
#include "common/types/basic.h"

#define ARP_CACHE_MAX 50
typedef struct {
    uint8_t paddr[IPV4_ADDR_SIZE];
    uint8_t haddr[ETH_HWA_SIZE];

    enum {
        NET_ARP_FREE = 0x1234,
        NET_ARP_RESOLVED,
        NET_ARP_WAITING,
    } state;
    int tmo;
    int rety;
    netif_t *netif;
    nlist_node_t node;
    nlist_t buf_list;
} arp_entry_t;

nlist_t cache_list;                   // 动态表
#define ARP_CACHE_SIZE 50
arp_entry_t cache_tbl[ARP_CACHE_SIZE];
static mblock_t cache_mblock;                // 空闲arp分配结构
void init_arp() {
    nlist_init(&cache_list);
    memset(cache_tbl, 0, sizeof(cache_tbl));
    mblock_init(&cache_mblock, cache_tbl, sizeof(arp_entry_t), ARP_CACHE_SIZE);
}

static arp_entry_t* cache_find(uint8_t* ip) {
    nlist_node_t* node;
    nlist_for_each(node, &cache_list) {
        arp_entry_t* entry = nlist_entry(node, arp_entry_t, node);
        if (memcmp(ip, entry->paddr, IPV4_ADDR_SIZE) == 0) {
            // 从表中移除，然后插入到表头，这样可使得最新使用的表项位于表头
            // 这样下次大概率也会使用该表项，以便节省下次查找的时间
            nlist_remove(&cache_list, node);
            nlist_insert_first(&cache_list, node);
            return entry;
        }
    }

    return (arp_entry_t*)0;
}

static void cache_clear_all(arp_entry_t *entry) {
    nlist_node_t *first;
    while((first = nlist_remove_first(&entry->buf_list))) {
        pktbuf_t *buf = nlist_entry(first, pktbuf_t, node);
        pktbuf_free(buf);
    }
}
static arp_entry_t* cache_alloc(int force) {
    arp_entry_t *entry = mblock_alloc(&cache_mblock, -1);
    if (!entry && force) {
        nlist_node_t *node = nlist_remove_last(&cache_list);
        if (!node) {
            return (arp_entry_t*)NULL;
        }
        entry = nlist_entry(node, arp_entry_t, node);
        cache_clear_all(entry);
    }
    if (entry) {
        memset(entry, 0, sizeof(arp_entry_t));
        nlist_node_init(&entry->node);
        nlist_init(&entry->buf_list);
    }
    return entry;
}

static void cache_entry_set(arp_entry_t *entry, const uint8_t *hw_addr, const uint8_t *ip_addr, netif_t *netif, int state) {
    memcpy(entry->haddr, hw_addr, ETH_HWA_SIZE);
    memcpy(entry->paddr, ip_addr, IPV4_ADDR_SIZE);
    entry->state = state;
    entry->netif = netif;
    if (state == NET_ARP_RESOLVED) {
        entry->tmo = 20*60;
    } else {
        entry->tmo = 3;
    }
    entry->rety = 5;
}

net_err_t arp_pkt_check(arp_pkt_t *arp_packet, int size, netif_t *netif) {
    if (size < sizeof(arp_packet)) {
        return NET_ERR_SIZE;
    }
    if (x_htons(arp_packet->htype) != ARP_HW_ETHER ||
        arp_packet->hlen != ETH_HWA_SIZE ||
        x_htons(arp_packet->ptype) != NET_PROTOCOL_IPv4 ||
        arp_packet->plen != IPV4_ADDR_SIZE
    ) {
        return NET_ERR_NOT_SUPPORT;
    }
    uint32_t opcode = x_htons(arp_packet->opcode);
    if (opcode != ARP_REQUEST && opcode != ARP_REPLY) {
        return NET_ERR_NOT_SUPPORT;
    }
    return NET_ERR_OK;
}

void show_arp_pkt(arp_pkt_t *arp_packet) {
    log_debug("------------------arp_start------------------\n");
    log_debug("    htype:%x\n", x_htons(arp_packet->htype));
    log_debug("    ptype:%x\n", x_htons(arp_packet->ptype));
    log_debug("    hlen:%x\n", x_htons(arp_packet->hlen));
    log_debug("    plen:%x\n", x_htons(arp_packet->plen));
    log_debug("    type:%x\n", x_htons(arp_packet->opcode));
    switch (x_htons(arp_packet->opcode)) {
        case ARP_REQUEST:
        log_debug("    request\n");
        break;
        case ARP_REPLY:
        log_debug("    reply\n");
        break;
        default:
        log_debug("    unknown");
        break;
    }

    char *mac = "ff:ff:ff:ff:ff:ff";
    char *ip = "\0\0\0.\0\0\0.\0\0\0.\0\0\0";
    mac2str(arp_packet->send_haddr, mac);
    ip2str(arp_packet->send_paddr, ip);
    log_debug("    sender_mac:%s", mac);
    log_debug("    sender_ip:%s\n", ip);
    mac2str(arp_packet->target_haddr, mac);
    ip2str(arp_packet->target_paddr, ip);
    log_debug("    target_mac:%s", mac);
    log_debug("    target_ip:%s\n", ip);
    log_debug("------------------arp_end------------------\n");
}

static net_err_t cache_send_all(arp_entry_t *entry) {
    nlist_node_t *first;
    while ((first = nlist_remove_first(&entry->buf_list))) {
        pktbuf_t *buf = nlist_entry(first, pktbuf_t, node);
        net_err_t err = ether_raw_out(entry->netif, NET_PROTOCOL_IPv4, entry->haddr, buf);
        if (err < 0) {
            pktbuf_free(buf);
            return err;
        }
    }
    return NET_ERR_OK;
}

void show_arp_tbl() {
    log_debug("------------------arp_table_start------------------\n");
    arp_entry_t *entry = cache_tbl;
    for (int i = 0; i < ARP_CACHE_SIZE; i++,entry++) {
        if (entry->state != NET_ARP_FREE && entry->state != NET_ARP_RESOLVED) {
            continue;
        }
        char *ip = "255.255.255.255";
        char *mac = "ff:ff:ff:ff:ff:ff";
        ip2str(entry->paddr, ip);
        mac2str(entry->haddr, mac);
        log_debug("    ip:%s    mac:%s\n", ip, mac);

    }
    log_debug("------------------arp_table_end------------------\n");
}

net_err_t cache_insert(netif_t *netif, uint8_t* ip_addr, uint8_t *hw_addr, int force) {
    arp_entry_t *entry = cache_find(ip_addr);
    if (!entry) {
        entry = cache_alloc(force);
        if (!entry) {
            return NET_ERR_NONE;
        }
        cache_entry_set(entry, hw_addr, ip_addr, netif, NET_ARP_RESOLVED);
        nlist_insert_first(&cache_list, &entry->node);
    } else {
        cache_entry_set(entry, hw_addr, ip_addr, netif, NET_ARP_RESOLVED);
        if (nlist_first(&cache_list) !=  &entry->node) {
            nlist_remove(&cache_list, &entry->node);
            nlist_insert_first(&cache_list, &entry->node);
        }
        net_err_t err = cache_send_all(entry);
        if (err < 0) {
            return err;
        }
    }
    show_arp_tbl();
    return NET_ERR_OK;
}

net_err_t arp_make_reply(netif_t *netif, pktbuf_t *buf) {
    arp_pkt_t *arp_packet = (arp_pkt_t*)pktbuf_data(buf);
    arp_packet->opcode = x_htons(ARP_REPLY);
    memcpy(arp_packet->target_haddr, arp_packet->send_haddr, ETH_HWA_SIZE);
    memcpy(arp_packet->target_paddr, arp_packet->send_paddr, IPV4_ADDR_SIZE);
    memcpy(arp_packet->send_haddr, netif->mac, ETH_HWA_SIZE);
    ip_addr_to_buf(&netif->ip_desc.ip_addr, arp_packet->send_paddr);
    show_arp_pkt(arp_packet);
    return ether_raw_out(netif, NET_PROTOCOL_ARP, arp_packet->target_haddr, buf);
}

net_err_t arp_in(netif_t *netif, pktbuf_t *buf) {
    net_err_t err = pktbuf_set_cont(buf, sizeof(arp_pkt_t));
    if (err < 0) {
        return err;
    }
    arp_pkt_t *arp_packet = (arp_pkt_t*)pktbuf_data(buf);
    err = arp_pkt_check(arp_packet, buf->total_size, netif);
    if (err != NET_ERR_OK) {
        return err;
    }
    show_arp_pkt(arp_packet);

    ip_addr_t target_ip;
    ip_addr_from_buf(&target_ip, arp_packet->target_paddr);
    if (target_ip.q_addr == netif->ip_desc.ip_addr.q_addr) {
        log_debug("received an arp, force update\n");
        cache_insert(netif, arp_packet->send_paddr, arp_packet->send_haddr, 1);
        if (x_htons(arp_packet->opcode) == ARP_REQUEST) {
            err = arp_make_reply(netif, buf);
        }
        return err;
    } else {
        cache_insert(netif, arp_packet->send_paddr, arp_packet->send_haddr, 0);
    }
    pktbuf_free(buf);
    return NET_ERR_OK;
}

net_err_t arp_update_from_ipbuf(netif_t *netif, pktbuf_t *buf) {
    net_err_t err = pktbuf_set_cont(buf, sizeof(ipv4_hdr_t) + sizeof(ether_hdr_t));
    if (err < 0) {
        return err;
    }
    ether_hdr_t *eth_hdr = (ether_hdr_t*)pktbuf_data(buf);
    ipv4_hdr_t *ip_hdr = (ipv4_hdr_t*)((uint8_t*)eth_hdr+sizeof(ether_hdr_t));

    if (ip_hdr->version != NET_VERSION_IPV4) {
        return NET_ERR_NOT_SUPPORT;
    }
    int total_size = x_htons(ip_hdr->total_len);
    if (total_size < sizeof(ipv4_hdr_t) || buf->total_size < total_size) {
        return NET_ERR_SIZE;
    }
    ip_addr_t dest_ip;
    ip_addr_from_buf(&dest_ip, ip_hdr->dest_ip);
    if (ipaddr_is_match(&netif->ip_desc.ip_addr, &dest_ip, &netif->ip_desc.net_mask)) {
        cache_insert(netif, ip_hdr->src_ip, eth_hdr->src, 0);
    }
}

uint8_t* arp_find(ip_addr_t *ip) {
    arp_entry_t *entry = cache_find(ip->a_addr);
    if (!entry) {
        return NULL;
    }
    return entry->haddr;
}

net_err_t arp_resolve(netif_t *netif, ip_addr_t *addr, pktbuf_t *buf) {
    arp_entry_t *entry = cache_find(addr->a_addr);
    if (entry) {
        if (entry->state == NET_ARP_RESOLVED) {
            return ether_raw_out(netif, NET_PROTOCOL_IPv4, entry->haddr, buf);
        } else if (nlist_count(&entry->buf_list) > ARP_CACHE_MAX) {
            return NET_ERR_FULL;
        } else {
            nlist_insert_first(&entry->buf_list, &buf->node);
            return NET_ERR_OK;
        }
    }
    entry = cache_alloc(1);
    if (!entry) {
        return NET_ERR_MEM;
    }
    cache_entry_set(entry, NULL, addr->a_addr, netif, NET_ARP_WAITING);
    nlist_insert_first(&entry->buf_list, &buf->node);
    nlist_insert_last(&cache_list, &entry->node);
    return arp_make_request(netif, addr);
}
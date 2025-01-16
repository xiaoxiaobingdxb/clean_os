#include "ip.h"
#include "netif.h"
#include "common/lib/hash.h"
#include "glibc/include/malloc.h"
#include "ether.h"
#include "icmp.h"
#include "common/tool/log.h"

#define IPV4_ADDR_BROADCAST 0xffffffff

#define RouteTableBits 10
//static hlist_head_t route_table[1 >> 10];
static nlist_t rt_list;                          // 路由表
static ip_route_entry_t rt_table[255];      // 路由表

#define  NET_IP_DEF_TTL 64

void init_ip() {
    nlist_init(&rt_list);
//    for (int i = 0; i < sizeof(route_table)/sizeof(hlist_head_t); i++) {
//        INIT_HLIST_HEAD(&route_table[i]);
//    }
}

int ipv4_hdr_size(ipv4_hdr_t *hdr) {
    return hdr->shdr * 4;
}

void show_ip_packet(ipv4_hdr_t *hdr) {
    log_debug("------------------ip_start------------------\n");
    log_debug("    protocol:%d\n", hdr->protocol);
    log_debug("    version:%d\n", hdr->version);
    log_debug("    header_len:%d\n", ipv4_hdr_size(hdr));
    log_debug("    total_len:%d\n", hdr->total_len);
    log_debug("    id:%d\n", hdr->id);
    log_debug("    frag_offset:%d\n", hdr->offset);
    log_debug("    frag_more:%d\n", hdr->more);
    log_debug("    ttl:%d\n", hdr->ttl);
    log_debug("    check_sum:%d\n", hdr->hdr_checksum);
    char *ip = "255.255.255.255";
    ip2str(hdr->src_ip, ip);
    log_debug("    src_ip:%s", ip);
    ip2str(hdr->dest_ip, ip);
    log_debug("    dest_ip:%s\n", ip);
    log_debug("------------------ip_end------------------\n");
}

net_err_t  ipv4_normal_in(netif_t *netif, ip_addr_t *src, ip_addr_t *desc,  pktbuf_t *buf) {
    ipv4_pkt_t *pkt = (ipv4_pkt_t*) pktbuf_data(buf);
    switch (pkt->hdr.protocol) {
        case NET_PROTOCOL_ICMPv4:
            return icmp_in(src, &netif->ip_desc.ip_addr, buf);
        case NET_PROTOCOL_UDP:
            break;
        case NET_PROTOCOL_TCP:
            break;
        default:
            return NET_ERR_NOT_SUPPORT;
    }
    return NET_ERR_NOT_SUPPORT;
}

net_err_t  ipv4_frag_in(netif_t *netif, pktbuf_t *buf) {
    return NET_ERR_OK;
}

static void iphdr_htons(ipv4_hdr_t * hdr) {
    hdr->total_len = x_htons(hdr->total_len);
    hdr->id = x_htons(hdr->id);
    hdr->frag_all = x_htons(hdr->frag_all);
}

net_err_t ipv4_in(netif_t *netif, pktbuf_t *buf) {
    net_err_t  err = pktbuf_set_cont(buf, sizeof(ipv4_hdr_t));
    if (err < 0) {
        return err;
    }
    ipv4_hdr_t  *hdr = (ipv4_hdr_t *)pktbuf_data(buf);
    iphdr_htons(hdr);

    err = pktbuf_resize(buf, hdr->total_len);
    if (err < 0) {
        return err;
    }

    ip_addr_t dest_ip;
    ip_addr_t src_ip;
    ip_addr_from_buf(&dest_ip, hdr->dest_ip);
    ip_addr_from_buf(&src_ip, hdr->src_ip);
    if (!ipaddr_is_match(&dest_ip, &netif->ip_desc.ip_addr, &netif->ip_desc.net_mask)) {
        pktbuf_free(buf);
        return NET_ERR_NOT_MATCH;
    }
    show_ip_packet(hdr);
    if (hdr->more || hdr->offset){
        return ipv4_frag_in(netif, buf);
    } else {
        return ipv4_normal_in(netif, &src_ip, &dest_ip, buf);
    }
}

net_err_t  ipv4_out(uint8_t protocol, ip_addr_t *dest, ip_addr_t *src, pktbuf_t *buf) {
    ip_route_entry_t *rt = route_find(dest);
    if (rt == NULL) {
        return NET_ERR_UNREACH;
    }
    ip_addr_t next_hop;
    if (rt->next_hop.q_addr == 0) {
        ipaddr_copy(&next_hop, dest);
    } else {
        ipaddr_copy(&next_hop, &rt->next_hop);
    }
    net_err_t err = pktbuf_add_header(buf, sizeof(ipv4_hdr_t), 1);
    if (err < 0) {
        return err;
    }
    ipv4_pkt_t *pkt = (ipv4_pkt_t*) pktbuf_data(buf);
    pkt->hdr.shdr_all = 0;
    pkt->hdr.version = NET_VERSION_IPV4;
    pkt->hdr.shdr = sizeof(ipv4_hdr_t)/4;
    pkt->hdr.total_len = buf->total_size;
    pkt->hdr.id = packet_id++;
    pkt->hdr.frag_all = 0;
    pkt->hdr.ttl = NET_IP_DEF_TTL;
    pkt->hdr.protocol = protocol;
    pkt->hdr.hdr_checksum = 0;
    if (!src || src->q_addr == 0) {
        ip_addr_to_buf(&rt->netif->ip_desc.ip_addr, pkt->hdr.src_ip);
    } else {
        ip_addr_to_buf(src, pkt->hdr.src_ip);
    }
    ip_addr_to_buf(dest, pkt->hdr.dest_ip);
    iphdr_htons(&pkt->hdr);
    pktbuf_reset_acc(buf);
    pkt->hdr.hdr_checksum = pktbuf_checksum16(buf, pkt->hdr.shdr * 4, 0, 1);
    err = netif_out(rt->netif, &next_hop, buf);
    return err;
}

void route_add(ip_desc_t *desc, ip_addr_t *next_hop, struct _netif_t *netif){
    ip_route_entry_t *entry = (ip_route_entry_t*)kernel_mallocator.malloc(sizeof(ip_route_entry_t));
    entry->desc.ip_addr.q_addr = desc->ip_addr.q_addr;
    entry->desc.net_mask.q_addr = desc->net_mask.q_addr;
    entry->desc.gateway.q_addr = desc->gateway.q_addr;
    entry->next_hop.q_addr = next_hop->q_addr;
    entry->netif = netif;
//    uint32_t hash_key = hash_long(desc->ip_addr.q_addr, RouteTableBits);
//    hlist_add_head(&entry->node, &route_table[hash_key]);
    nlist_insert_last(&rt_list, &entry->node);
    if (next_hop->q_addr == netif->ip_desc.ip_addr.q_addr) {
        entry->type = NET_RT_NETIF;
    } else if (!next_hop) {
        entry->type = NET_RT_LOCAL_NET;
    } else {
        entry->type = NET_RT_OTHER;
    }
}
void route_remove(ip_desc_t *desc) {

}
uint32_t ipaddr_get_net(const ip_addr_t * ipaddr, const ip_addr_t * netmask);
ip_route_entry_t* route_find(ip_addr_t *ip) {
    ip_route_entry_t* e = (ip_route_entry_t *)0;

    nlist_node_t* node;

    // 遍历整个列表
    nlist_for_each(node, &rt_list) {
        ip_route_entry_t * entry = nlist_entry(node, ip_route_entry_t, node);

        // ip & mask != entry->net就跳过
        uint32_t net = ipaddr_get_net(ip, &entry->desc.net_mask);
        if (net != entry->desc.ip_addr.q_addr) {
            continue;
        }

        // 找到匹配项时，做以下检查
        // 如果是第一次找到匹配的项，或者掩码中1的数量更多，即更匹配，则使用该项
        if (!e /*|| (e->mask_1_cnt < entry->mask_1_cnt)*/) {
            e = entry;
        }
    }

    return e;
    /*
    uint32_t hash_key = hash_long(ip->q_addr, RouteTableBits);
    ip_route_entry_t *entry;
    hlist_node_t *node;
    hlist_for_each_entry(entry, node, &route_table[hash_key], node) {
        if (entry->desc.ip_addr.q_addr == ip->q_addr & entry->desc.net_mask.q_addr) {
            return entry;
        }
    }
    return (ip_route_entry_t*)NULL;
     */
}

void ip_addr_to_buf(ip_addr_t *ip, uint8_t* ip_buf) {
    for (int i = 0; i < sizeof(ip->a_addr)/sizeof(uint8_t); i++) {
        ip_buf[i] = ip->a_addr[i];
    }
}

void ip_addr_from_buf(ip_addr_t *ip, uint8_t *ip_buf) {
    for (int i = 0; i < sizeof(ip->a_addr)/sizeof(uint8_t); i++) {
         ip->a_addr[i] = ip_buf[i];
    }
}

net_err_t ipaddr_from_str(ip_addr_t * dest, const char* str) {
    // 必要的参数检查
    if (!dest || !str) {
        return NET_ERR_PARAM;
    }

    // 初始值清空
    dest->q_addr = 0;

    // 不断扫描各字节串，直到字符串结束
    char c;
    uint8_t * p = dest->a_addr;
    uint8_t sub_addr = 0;
    while ((c = *str++) != '\0') {
        // 如果是数字字符，转换成数字并合并进去
        if ((c >= '0') && (c <= '9')) {
            // 数字字符转换为数字，再并入计算
            sub_addr = sub_addr * 10 + c - '0';
        } else if (c == '.') {
            // . 分隔符，进入下一个地址符，并重新计算
            *p++ = sub_addr;
            sub_addr = 0;
        } else {
            // 格式错误，包含非数字和.字符
            return NET_ERR_PARAM;
        }
    }

    // 写入最后计算的值
    *p++ = sub_addr;
    return NET_ERR_OK;
}

ip_addr_t* ipaddr_get_any() {
    static ip_addr_t ip = {.q_addr = 0};
    return &ip;
}

void ipaddr_copy(ip_addr_t * dest, const ip_addr_t * src) {
    if (!dest || !src) {
        return;
    }

    dest->q_addr = src->q_addr;
}

uint32_t ipaddr_get_host(const ip_addr_t * ipaddr, const ip_addr_t * netmask) {
    return ipaddr->q_addr & ~netmask->q_addr;
}
uint32_t ipaddr_get_net(const ip_addr_t * ipaddr, const ip_addr_t * netmask) {
    ip_addr_t netid;

    netid.q_addr = ipaddr->q_addr & netmask->q_addr;
    return netid.q_addr;
}
int ipaddr_is_direct_broadcast(const ip_addr_t * ipaddr, const ip_addr_t * netmask) {
    uint32_t hostid = ipaddr_get_host(ipaddr, netmask);

    // 判断host_id部分是否为全1
    return hostid == (IPV4_ADDR_BROADCAST & ~netmask->q_addr);
}

bool ipaddr_is_match(const ip_addr_t* dest, const ip_addr_t *src, const ip_addr_t *netmask) {
    if (dest->q_addr == IPV4_ADDR_BROADCAST) {
        return true;
    }
    if (ipaddr_is_direct_broadcast(dest, netmask) && ipaddr_get_net(dest, netmask) == ipaddr_get_net(src, netmask)) {
        return true;
    }
    return dest->q_addr == src->q_addr;
}
#include "ip.h"
#include "netif.h"
#include "common/lib/hash.h"
#include "glibc/include/malloc.h"

#define IPV4_ADDR_BROADCAST 0xffffffff

#define RouteTableBits 10
hlist_head_t route_table[1 >> 10];

void init_ip() {
    for (int i = 0; i < sizeof(route_table)/sizeof(hlist_head_t); i++) {
        INIT_HLIST_HEAD(&route_table[i]);
    }
}

void route_add(ip_desc_t *desc, ip_addr_t *next_hop, struct _netif_t *netif){
    ip_route_entry_t *entry = (ip_route_entry_t*)kernel_mallocator.malloc(sizeof(ip_route_entry_t));
    entry->desc.ip_addr.q_addr = desc->ip_addr.q_addr;
    entry->desc.net_mask.q_addr = desc->net_mask.q_addr;
    entry->desc.gateway.q_addr = desc->gateway.q_addr;
    entry->next_hop.q_addr = next_hop->q_addr;
    entry->netif = netif;
    uint32_t hash_key = hash_long(desc->ip_addr.q_addr, RouteTableBits);
    hlist_add_head(&entry->node, &route_table[hash_key]);
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
ip_route_entry_t* route_find(ip_addr_t *ip) {
    uint32_t hash_key = hash_long(ip->q_addr, RouteTableBits);
    ip_route_entry_t *entry;
    hlist_node_t *node;
    hlist_for_each_entry(entry, node, &route_table[hash_key], node) {
        if (entry->desc.ip_addr.q_addr == ip->q_addr & entry->desc.net_mask.q_addr) {
            return entry;
        }
    }
    return (ip_route_entry_t*)NULL;
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
ip_addr_t ipaddr_get_net(const ip_addr_t * ipaddr, const ip_addr_t * netmask) {
    ip_addr_t netid;

    netid.q_addr = ipaddr->q_addr & netmask->q_addr;
    return netid;
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
    if (ipaddr_is_direct_broadcast(dest, netmask) && ipaddr_get_net(dest, netmask).q_addr == ipaddr_get_net(src, netmask).q_addr) {
        return true;
    }
    return dest->q_addr == src->q_addr;
}
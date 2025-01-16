#if !defined(NET_IP_H)
#define NET_IP_H

#include "common/types/basic.h"
#include "common/lib/hlist.h"
#include "net_err.h"
#include "pktbuf.h"

#define IPV4_ADDR_SIZE 4
#define NET_VERSION_IPV4        4           // IPV4版本号

static uint16_t packet_id = 0;                  // 每次发包的序号

typedef struct {
    enum {
        IPADDR_V4,
    } type;
    union
    {
        uint32_t q_addr;
        uint8_t a_addr[4];
    };
    char s_addr[15]; // 192.168.0001.0001

} ip_addr_t;

typedef struct {
    ip_addr_t ip_addr;
    ip_addr_t net_mask;
    ip_addr_t gateway;
} ip_desc_t;

struct _netif_t;
typedef struct {
    ip_desc_t desc;
    ip_addr_t next_hop;

    struct _netif_t *netif;

    enum {
        NET_RT_LOCAL_NET,
        NET_RT_NETIF,
        NET_RT_OTHER,
        lis
    } type;

//    hlist_node_t node;
    nlist_node_t node;
} ip_route_entry_t;

#pragma pack(1)

typedef struct {
    union
    {
        struct {
            uint16_t shdr: 4;
            uint16_t version: 4;
            uint16_t ds: 6;
            uint16_t ecn: 2;
        };
        uint16_t shdr_all;
    };
    uint16_t total_len;
    uint16_t id;
    union
    {
        struct
        {
            uint16_t offset: 13;
            uint16_t more: 1;
            uint16_t disable: 1;
            uint16_t resvered: 1;
        };
        uint16_t frag_all;
    };
    uint8_t ttl;
    uint8_t protocol;
    uint16_t hdr_checksum;
    uint8_t src_ip[IPV4_ADDR_SIZE];
    uint8_t dest_ip[IPV4_ADDR_SIZE];

} ipv4_hdr_t;
typedef struct {
    ipv4_hdr_t hdr;
    uint8_t data[1];
} ipv4_pkt_t;
#pragma pack()

void init_ip();
net_err_t ipv4_in(struct _netif_t *netif, pktbuf_t *buf);
net_err_t  ipv4_out(uint8_t protocol, ip_addr_t *dest, ip_addr_t *src, pktbuf_t *buf);
void route_add(ip_desc_t *desc, ip_addr_t *next_hop, struct _netif_t *netif);
void route_remove(ip_desc_t *desc);
ip_route_entry_t* route_find(ip_addr_t *ip);

void ip_addr_to_buf(ip_addr_t *ip, uint8_t* ip_buf);
void ip_addr_from_buf(ip_addr_t *ip, uint8_t *ip_buf);

net_err_t ipaddr_from_str(ip_addr_t * dest, const char* str);
ip_addr_t* ipaddr_get_any();
void ipaddr_copy(ip_addr_t * dest, const ip_addr_t * src);

bool ipaddr_is_match(const ip_addr_t* dest, const ip_addr_t *src, const ip_addr_t *netmask);
#endif // NET_IP_H

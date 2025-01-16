#if !defined(NET_ICMP_H)
#define NET_ICMP_H

#include "net_err.h"
#include "netif.h"

net_err_t icmp_in(ip_addr_t  *src, ip_addr_t *dest, pktbuf_t *buf);
net_err_t icmp_out(ip_addr_t *src, ip_addr_t *dest, pktbuf_t *buf);
#endif
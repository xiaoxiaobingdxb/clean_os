#include "net.h"
#include "ether.h"
#include "netif.h"
#include "driver/rtl8139.h"
#include "../thread/thread.h"
#include "exmsg.h"
#include "arp.h"

net_err_t netif_set_addr(netif_t *netif, ip_addr_t *ip, ip_addr_t *netmask, ip_addr_t *gateway) {
    ipaddr_copy(&netif->ip_desc.ip_addr, ip ? ip : ipaddr_get_any());
    ipaddr_copy(&netif->ip_desc.net_mask, netmask ? netmask : ipaddr_get_any());
    ipaddr_copy(&netif->ip_desc.gateway, gateway ? gateway : ipaddr_get_any());
    return NET_ERR_OK;
}

extern const netif_ops_t netdev8139_ops;
static rtl8139_priv_t priv;

net_err_t init_netdev() {
    netif_t *netif = netif_open("netif0", &netdev8139_ops, &priv);
    if (!netif) {
        return NET_ERR_IO;
    }
static const char netdev0_ip[] = "10.3.208.99";
static const char netdev0_gw[] = "10.3.208.1";
//    static const char netdev0_ip[] = "192.168.1.99";
//    static const char netdev0_gw[] = "192.168.1.1";
    static const char netdev0_mask[] = "255.255.224.0";
    // 生成相应的地址
    ip_addr_t ip, mask, gw;
    ipaddr_from_str(&ip, netdev0_ip);
    ipaddr_from_str(&mask, netdev0_mask);
    ipaddr_from_str(&gw, netdev0_gw);
    netif_set_addr(netif, &ip, &mask, &gw);
    netif_set_active(netif);

    // 使用接口2作为缺省的接口
    netif_set_default(netif);
    return NET_ERR_OK;
}

extern void net_work_thread(void *args);

void net_start() {
    thread_start("net_task", 32, net_work_thread, NULL);
}

net_err_t init_net() {
    exmsg_init();
    pktbuf_init();
    init_netif();
    init_ether();
    init_arp();

    init_netdev();

    net_start();
}

#if !defined(NET_DRIVER_H)
#define NET_DRIVER_H

#include "common/types/basic.h"
#include "../netif.h"


void init_e1000();

static void init_net_driver() {
    // init_e1000();
    // init_rtl8139();
}

net_err_t rtl8139_open(netif_t* p_netif, void* ops_data);
net_err_t rtl8139_xmit (struct _netif_t* netif);

#endif // NET_DRIVER_H

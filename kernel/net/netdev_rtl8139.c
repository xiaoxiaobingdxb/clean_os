#include "netif.h"
#include "driver/driver.h"

const netif_ops_t netdev8139_ops = {
    .open = rtl8139_open,
    .xmit = rtl8139_xmit,
};
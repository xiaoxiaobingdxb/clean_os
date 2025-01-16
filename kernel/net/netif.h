#if !defined(NET_NETIF_H)
#define NET_NETIF_H

#include "common/types/basic.h"
#include "net_err.h"
#include "ip.h"
#include "pktbuf.h"
#include "common/lib/list.h"
#include "fixq.h"

typedef enum {
    NETIF_TYPE_NONE = 0,
    NETIF_TYPE_ETHER,
    NETIF_TYPE_LOOP,
    NETIF_TYPE_SIZE,
} netif_type_t;

struct _netif_t;
typedef struct {
    netif_type_t type;
    net_err_t (*open)(struct _netif_t *netif);
    void (*close)(struct _netif_t *netif);
    net_err_t (*in)(struct _netif_t *netif, pktbuf_t *buf);
    net_err_t (*out)(struct _netif_t *netif, ip_addr_t *dest, pktbuf_t *buf);
} link_layer_t;

struct _netif_t;
typedef struct {
    net_err_t (*open)(struct _netif_t *netif, void *data);
    net_err_t (*close)(struct _netif_t *netif);

    net_err_t (*xmit)(struct _netif_t* netif);
} netif_ops_t;

#define NETIF_INQ_SIZE                  50                  // 网卡输入队列最大容量
#define NETIF_OUTQ_SIZE                 50                  // 网卡输出队列最大容量
typedef struct _netif_t {
    char name[10];
    uint8_t mac[6];
    ip_desc_t ip_desc;
    enum {
        NETIF_CLOSED,
        NETIF_OPENED,
        NETIF_ACTIVE,
    } state;
    netif_type_t type;
    int mtu;
    const netif_ops_t* ops;
    void *ops_data;
    const link_layer_t *link_layer;

    list_node node;

    fixq_t in_q;                            // 数据包输入队列
    void * in_q_buf[NETIF_INQ_SIZE];
    fixq_t out_q;                           // 数据包发送队列
    void * out_q_buf[NETIF_OUTQ_SIZE];
} netif_t;

void init_netif();

net_err_t netif_register_layer(netif_type_t type, const link_layer_t *layer);
const link_layer_t* netif_get_layer(netif_type_t type);
netif_t* netif_open(const char *dev_name, const netif_ops_t *ops, void *ops_data);
void netif_set_default(netif_t* netif);
net_err_t netif_set_active(netif_t *netif);


// 数据包输入输出管理
net_err_t netif_put_in(netif_t* netif, pktbuf_t* buf, int tmo);
net_err_t netif_put_out(netif_t * netif, pktbuf_t * buf, int tmo);
pktbuf_t* netif_get_in(netif_t* netif, int tmo);
pktbuf_t* netif_get_out(netif_t * netif, int tmo);
net_err_t netif_out(netif_t* netif, ip_addr_t* ipaddr, pktbuf_t* buf);

#endif // NET_NETIF_H

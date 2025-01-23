//
// Created by ByteDance on 2025/1/17.
//

#include "sock.h"
#include "ip.h"
#include "netif.h"
#include "common/lib/string.h"
#include "udp.h"

typedef struct {
    enum {
        SOCKET_STATE_FREE,
        SOCKET_STATE_USED
    } state;
    sock_t *sock;
} socket_t;
#define SOCKET_MAX_NR        100
static socket_t socket_tbl[SOCKET_MAX_NR];

net_err_t init_socket() {
    memset(socket_tbl, 0, sizeof(socket_tbl));
    return NET_ERR_OK;
}

int socket_index(socket_t *socket) {
    return socket - socket_tbl;
}

socket_t *get_socket(int idx) {
    if ((idx < 0) || (idx >= SOCKET_MAX_NR)) {
        return (socket_t *) NULL;
    }

    socket_t *s = socket_tbl + idx;
    return s->sock == (sock_t *) NULL ? (socket_t *) NULL : s;
}

net_err_t sock_init(sock_t *sock, int family, int protocol, sock_ops_t *ops) {
    sock->protocol = protocol;
    sock->ops = ops;
    sock->family = family;

    sock->local_ip.q_addr = 0;
    sock->local_port = 0;
    sock->remote_ip.q_addr = 0;
    sock->remote_port = 0;
    sock->err = NET_ERR_OK;
    sock->recv_tmo = 0;
    sock->send_tmo = 0;
    nlist_node_init(&sock->node);

    sock->recv_wait = NULL;
    sock->send_wait = NULL;
    sock->conn_wait = NULL;
    return NET_ERR_OK;
}

void sock_uninit(sock_t *sock) {
}

net_err_t sock_wait_init(sock_wait_t *wait) {
    wait->waiting = 0;
}

net_err_t sock_bind(sock_t *sock, const sock_addr_t *addr, sock_len_t len) {
    sock_addr_in_t *in_addr = (sock_addr_in_t *) addr;
    if (in_addr->in_addr.q_addr != 0) {
        ip_addr_t local_ip;
        ip_addr_from_buf(&local_ip, in_addr->in_addr.a_addr);
        ip_route_entry_t *entry = route_find(&local_ip);
        if (!entry) {
            return NET_ERR_PARAM;
        }
        netif_t *netif = entry->netif;
        if (netif->ip_desc.ip_addr.q_addr != local_ip.q_addr) {
            return NET_ERR_PARAM;
        }
    }
    ip_addr_from_buf(&sock->local_ip, in_addr->in_addr.a_addr);
    sock->local_port = in_addr->sin_port;
    return NET_ERR_OK;
}

void sock_wait_leave(sock_wait_t *wait, net_err_t err) {
    if (wait->waiting > 0) {
        wait->waiting--;
        wait->err = err;
        segment_wakeup(&wait->sem, NULL);
    }
}

void sock_wait_add(sock_wait_t *wait, int tmo, sock_req_t *req) {
    req->wait = wait;
    req->wait_tmo = tmo;
    wait->waiting++;
}

net_err_t sock_wait_enter(sock_wait_t *wait, int tmo) {
    segment_wait(&wait->sem, NULL);
    return wait->err;
}

void sock_wakeup(sock_t *sock, int type, int err) {
    if (type & SOCK_WAIT_CONN) {
        sock_wait_leave(sock->conn_wait, err);
    }

    if (type & SOCK_WAIT_WRITE) {
        sock_wait_leave(sock->send_wait, err);
    }

    if (type & SOCK_WAIT_READ) {
        sock_wait_leave(sock->recv_wait, err);
    }
    sock->err = err;
}

static socket_t *socket_alloc(void) {
    socket_t *s = (socket_t *) NULL;

    for (int i = 0; i < SOCKET_MAX_NR; i++) {
        socket_t *curr = socket_tbl + i;
        if (curr->state == SOCKET_STATE_FREE) {
            s = curr;
            s->state = SOCKET_STATE_USED;
            break;
        }
    }
    return s;
}

static void socket_free(socket_t* s) {
    s->sock = (sock_t *)0;
    s->state = SOCKET_STATE_FREE;
}

net_err_t handle_sock_create(func_msg_t *msg) {
    // 根据不同的协议创建不同的sock信息表
    static const struct sock_info_t {
        int protocol;			// 缺省的协议
        sock_t* (*create) (int family, int protocol);
    }  sock_tbl[] = {
//            [SOCK_RAW] = { .protocol = 0, .create = raw_create,},
            [SOCK_DGRAM] = { .protocol = IPPROTO_UDP, .create = udp_create,},
//            [SOCK_STREAM] = {.protocol = IPPROTO_TCP,  .create = tcp_create,},
    };

    sock_req_t * req = (sock_req_t *)msg->param;
    sock_create_t * param = &req->create;

    // 分配socket结构，建立连接
    socket_t * socket = socket_alloc();
    if (socket == (socket_t *)NULL) {
        return NET_ERR_MEM;
    }

    // 对type参数进行检查
    if ((param->type < 0) || (param->type >= sizeof(sock_tbl) / sizeof(sock_tbl[0]))) {
        socket_free(socket);
        return NET_ERR_PARAM;
    }

    // 根据协议，创建底层sock, 未指定协议，使用缺省的
    const struct sock_info_t* info = sock_tbl + param->type;
    if (param->protocol == 0) {
        param->protocol = info->protocol;
    }

    // 根据类型创建不同的socket
    sock_t * sock = info->create(param->family, param->protocol);
    if (!sock) {
        socket_free(socket);
        return NET_ERR_MEM;
    }

    socket->sock = sock;
    req->sock_fd = socket_index(socket);
    return NET_ERR_OK;
}

net_err_t handle_sock_bind(func_msg_t *msg) {
    sock_req_t *req = (sock_req_t *) msg->param;
    socket_t *socket = get_socket(req->sock_fd);
    if (!socket) {
        return NET_ERR_PARAM;
    }
    if (!socket->sock) {
        return NET_ERR_PARAM;
    }
    sock_bind_t *bind = (sock_bind_t *) &req->bind;
    return socket->sock->ops->bind(socket->sock, bind->addr, bind->len);
}

net_err_t handle_sock_send(func_msg_t *msg) {

}

net_err_t handle_sock_receive(func_msg_t *msg) {
    sock_req_t * req = (sock_req_t *)msg->param;
    socket_t * s = get_socket(req->sock_fd);
    if (!s) {
        return NET_ERR_PARAM;
    }
    sock_t* sock = s->sock;
    sock_data_t * data = (sock_data_t *)&req->data;

    net_err_t err = sock->ops->receive(sock, data->buf, data->len, data->flags, &req->data.comp_len);
    if (err == NET_ERR_NEED_WAIT) {
        if (sock->recv_wait) {
            sock_wait_add(sock->recv_wait, sock->recv_tmo, req);
        }
    }
    return err;
}

net_err_t handle_sock_set_opt(func_msg_t *msg) {

}
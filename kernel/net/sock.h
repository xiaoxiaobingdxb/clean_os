//
// Created by ByteDance on 2025/1/17.
//

#ifndef NET_SOCK_H
#define NET_SOCK_H

#include "net_err.h"
#include "nlist.h"
#include "ip.h"
#include "../thread/schedule/segment.h"
#include "include/fs_model.h"
#include "exmsg.h"
#include "include/socket.h"

#define SOCK_WAIT_READ         (1 << 0)
#define SOCK_WAIT_WRITE        (1 << 1)
#define SOCK_WAIT_CONN         (1 << 2)
#define SOCK_WAIT_ALL          (SOCK_WAIT_CONN |SOCK_WAIT_READ | SOCK_WAIT_WRITE)

typedef struct {
    int family;
    int protocol;
    int type;
} sock_create_t;
typedef struct {
    uint8_t *buf;
    size_t len;
    int flags;
    sock_addr_t *addr;
    sock_len_t *addr_len;
    ssize_t  comp_len;
} sock_data_t;
typedef struct {
    int level;
    int opt_name;
    const char *opt_value;
    int opt_len;
} sock_opt_t;
typedef struct {
    const sock_addr_t *addr;
    sock_len_t len;
} sock_bind_t;
typedef struct {
    int back_log;
} sock_listen_t;
typedef struct {
    const sock_addr_t *addr;
    sock_len_t len;
} sock_connect_t;
typedef struct {
    sock_addr_t *addr;
    sock_len_t *len;
    fd_t client_fd;
} sock_accept_t;

typedef struct {
    net_err_t err;
    int waiting;
    segment_t sem;
} sock_wait_t;

typedef struct {
    sock_wait_t *wait;
    int wait_tmo;

    fd_t sock_fd;
    union {
        sock_create_t create;
        sock_bind_t bind;
        sock_listen_t listen;
        sock_connect_t connect;
        sock_accept_t accept;
        sock_data_t data;
        sock_opt_t opt;
    };
} sock_req_t;

struct _sock_ops_t;
typedef struct {
    ip_addr_t local_ip;
    uint16_t local_port;
    ip_addr_t remote_ip;
    uint16_t remote_port;
    const struct _sock_ops_t *ops;
    int family;
    int protocol;
    int err;
    int recv_tmo;
    int send_tmo;

    sock_wait_t *recv_wait;
    sock_wait_t *send_wait;
    sock_wait_t *conn_wait;

    nlist_node_t node;
} sock_t;


typedef struct _sock_ops_t {
    net_err_t (*listen)(sock_t *sock, int backlog);

    net_err_t (*accept)(sock_t *sock, sock_addr_t *addr, sock_len_t *len, sock_t **client);

    net_err_t (*set_opt)(sock_t *sock, int level, int opt_name, const char *opt_val, int option);

    net_err_t (*bind)(sock_t *sock, const sock_addr_t *addr, sock_len_t len);

    net_err_t (*connect)(sock_t *sock, const sock_addr_t *addr, sock_len_t len);

    net_err_t (*send)(sock_t *sock, const void *buf, size_t len, int flags, ssize_t *result_len);

    net_err_t (*receive)(sock_t *sock, void *buf, size_t len, int falgs, ssize_t *result_len);

    net_err_t
    (*send_to)(sock_t *sock, const void *buf, size_t len, int falgs, const sock_addr_t *dest, sock_len_t dest_len,
               ssize_t *result_len);

    net_err_t (*receive_from)(sock_t *sock, void *buf, size_t len, int flags, sock_addr_t *src, sock_len_t addr_len,
                              ssize_t *result_len);

    net_err_t (*close)(sock_t *sock);

    net_err_t (*destroy)(sock_t *sock);
} sock_ops_t;

net_err_t init_socket();

net_err_t sock_init(sock_t *sock, int family, int protocol, sock_ops_t *ops);

void sock_uninit(sock_t *sock);

net_err_t sock_wait_init(sock_wait_t *wait);

net_err_t sock_bind(sock_t *sock, const sock_addr_t *addr, sock_len_t len);
net_err_t sock_connect(sock_t *sock, const sock_addr_t *addr, sock_len_t len);

net_err_t sock_send(sock_t *sock, const void *buf, size_t len, int flags, ssize_t *result_len);
net_err_t sock_receive(sock_t *sock, void *buf, size_t len, int flags, ssize_t *result_len);

net_err_t sock_wait_enter(sock_wait_t *wait, int tmo);
void sock_wakeup(sock_t *sock, int type, int err);


net_err_t handle_sock_create(func_msg_t *msg);
net_err_t handle_sock_bind(func_msg_t *msg);
net_err_t handle_sock_connect(func_msg_t *msg);
net_err_t handle_sock_send(func_msg_t *msg);
net_err_t handle_sock_receive(func_msg_t *msg);
net_err_t handle_sock_set_opt(func_msg_t *msg);
#endif //NET_SOCK_H

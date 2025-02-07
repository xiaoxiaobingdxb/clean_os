#include "raw.h"
#include "sock.h"
#include "mblock.h"
#include "ip.h"

typedef struct {
    sock_t sock;
    nlist_t recv_list;
    sock_wait_t recv_wait;
} raw_t;

#define RAW_MAX_NR 10
static raw_t raw_tbl[RAW_MAX_NR];
static mblock_t raw_mblock;
static nlist_t raw_list;

#define RAW_MAX_RECV 50

net_err_t init_raw() {
    mblock_init(&raw_mblock, raw_tbl, sizeof(raw_t), RAW_MAX_NR);
    nlist_init(&raw_list);
    return NET_ERR_OK;
}

raw_t *raw_find(ip_addr_t *src, ip_addr_t *dest, int protocol) {
    nlist_node_t * node;
    nlist_for_each(node, &raw_list) {
        raw_t *raw = (raw_t*)nlist_entry(node, sock_t , node);
        if (raw->sock.protocol != protocol) {
            continue;
        }
        if (raw->sock.local_ip.q_addr != 0 && raw->sock.local_ip.q_addr != dest->q_addr) {
            continue;
        }
        if (raw->sock.remote_ip.q_addr != 0 && raw->sock.remote_ip.q_addr != src->q_addr) {
            continue;
        }
        return raw;
    }
    return NULL;
}

net_err_t raw_in(pktbuf_t *buf) {
    ipv4_pkt_t *ip_pkt = (ipv4_pkt_t *) pktbuf_data(buf);
    ip_addr_t src, dest;
    ip_addr_from_buf(&src, ip_pkt->hdr.src_ip);
    ip_addr_from_buf(&dest, ip_pkt->hdr.dest_ip);
    // 提交给合适的sock
    raw_t *raw = (raw_t *) raw_find(&src, &dest, ip_pkt->hdr.protocol);
    if (!raw) {
        return NET_ERR_UNREACH;
    }

    // 写到接收队列，通知应用程序取
    if (nlist_count(&raw->recv_list) < RAW_MAX_RECV) {
        nlist_insert_last(&raw->recv_list, &buf->node);
        sock_wakeup((sock_t *) raw, SOCK_WAIT_READ, NET_ERR_OK);
    } else {
        pktbuf_free(buf);
    }
    return NET_ERR_OK;
}

net_err_t
raw_send_to(sock_t *sock, const void *buf, size_t len, int flags, const sock_addr_t *dest, sock_len_t dest_len,
            ssize_t *result_len) {
    sock_addr_in_t *addr_in = (sock_addr_in_t *) dest;
    if (sock->remote_ip.q_addr != 0 && sock->remote_ip.q_addr != addr_in->in_addr.q_addr) {
        return NET_ERR_CONNECTED;
    }
    pktbuf_t * pkt_buf = pktbuf_alloc(len);
    if (!buf) {
        return NET_ERR_MEM;
    }
    pktbuf_write(pkt_buf, (uint8_t * )
    buf, len);
    ip_addr_t dest_ip_addr, src_ip_addr;
    dest_ip_addr.type = IPADDR_V4;
    dest_ip_addr.q_addr = addr_in->in_addr.q_addr;
    src_ip_addr.type = IPADDR_V4;
    src_ip_addr.q_addr = sock->local_ip.q_addr;

    net_err_t err = ipv4_out(sock->protocol, &dest_ip_addr, &src_ip_addr, pkt_buf);
    if (err < 0) {
        goto send_fail;
    }
    *result_len = (ssize_t) len;
    return NET_ERR_OK;
    send_fail:
    pktbuf_free(pkt_buf);
    return err;
}

net_err_t raw_receive_from(sock_t *sock, void *buf, size_t len, int flags, sock_addr_t *src, sock_len_t addr_len,
                           ssize_t *result_len) {
    raw_t *raw = (raw_t *) sock;
    nlist_node_t * first = nlist_remove_first(&raw->recv_list);
    if (!first) {
        return NET_ERR_NEED_WAIT;
    }
    pktbuf_t * pkt_buf = nlist_entry(first, pktbuf_t, node);
    ipv4_hdr_t *ip_hdr = (ipv4_hdr_t *) pktbuf_data(pkt_buf);
    sock_addr_in_t *addr_in = (sock_addr_in_t *) src;
    memset(addr_in, 0, sizeof(sock_addr_in_t));
    addr_in->sin_family = AF_INET;
    addr_in->sin_port = 0;
    memcpy(addr_in->in_addr.a_addr, ip_hdr->src_ip, IPV4_ADDR_SIZE);
    size_t size = pkt_buf->total_size > len ? len : pkt_buf->total_size;
    pktbuf_reset_acc(pkt_buf);
    net_err_t err = pktbuf_read(pkt_buf, (uint8_t * )
    buf, size);
    if (err < 0) {
        pktbuf_free(buf);
        return err;
    }
    *result_len = (ssize_t) size;
    pktbuf_free(pkt_buf);
    return NET_ERR_OK;
}

net_err_t raw_close(sock_t *sock) {
    raw_t *raw = (raw_t *) sock;

    nlist_remove(&raw_list, &sock->node);

    nlist_node_t * node;
    while ((node = nlist_remove_first(&raw->recv_list))) {
        pktbuf_t * buf = nlist_entry(node, pktbuf_t, node);
        pktbuf_free(buf);
    }
    mblock_free(&raw_mblock, sock);
    return NET_ERR_OK;
}

sock_t *raw_create(int family, int protocol) {
    static sock_ops_t raw_ops = {
            .send_to = raw_send_to,
            .receive_from = raw_receive_from,
            .connect = sock_connect,
            .bind = sock_bind,
            .close = raw_close,
            .set_opt = sock_set_opt,
            .send = sock_send,
            .receive = sock_receive,
    };
    raw_t *raw = (raw_t *) mblock_alloc(&raw_mblock, 0);
    if (!raw) {
        return (sock_t *) NULL;
    }
    net_err_t err = sock_init((sock_t *) raw, family, protocol, &raw_ops);
    if (err < 0) {
        mblock_free(&raw_mblock, raw);
        return (sock_t *) NULL;
    }
    nlist_init(&raw->recv_list);
    raw->sock.recv_wait = &raw->recv_wait;
    if (sock_wait_init(raw->sock.recv_wait) < 0) {
        goto create_failed;
    }
    nlist_insert_last(&raw_list, &raw->sock.node);
    return (sock_t *) raw;
    create_failed:
    sock_uninit((sock_t *) raw);
    return (sock_t *) NULL;
}
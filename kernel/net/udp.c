//
// Created by ByteDance on 2025/1/17.
//

#include "udp.h"
#include "mblock.h"
#include "util.h"
#include "sock.h"
#include "common/tool/log.h"

#define UDP_MAX_RECV 50

// @ref https://datatracker.ietf.org/doc/html/rfc768
#pragma pack(1)
typedef struct {
    uint16_t src_port;
    uint16_t dest_port;
    uint16_t total_len;
    uint16_t check_sum;
} udp_hdr_t;

typedef struct {
    udp_hdr_t hdr;
    uint8_t data[1];
} udp_pkt_t;
#pragma pack(0)

typedef struct {
    ip_addr_t from;
    uint16_t port;
}udp_from_t;

typedef struct {
    sock_t sock;
    nlist_t recv_list;
    sock_wait_t recv_wait;
} udp_t;

#define UDP_MAX_NR 10
static udp_t udp_tbl[UDP_MAX_NR];
static mblock_t udp_mblock;
static nlist_t udp_list;

net_err_t init_udp() {
    mblock_init(&udp_mblock, udp_tbl, sizeof(udp_t), UDP_MAX_NR);
    nlist_init(&udp_list);
    return NET_ERR_OK;
}

net_err_t udp_close(sock_t *sock) {
    udp_t *udp = (udp_t *) sock;

    nlist_remove(&udp_list, &sock->node);

    nlist_node_t * node;
    while ((node = nlist_remove_first(&udp->recv_list))) {
        pktbuf_t * buf = nlist_entry(node, pktbuf_t, node);
        pktbuf_free(buf);
    }
    mblock_free(&udp_mblock, sock);
    return NET_ERR_OK;
}

net_err_t udp_set_opt(sock_t *sock, int level, int opt_name, const char *opt_val, int option) {
    return NET_ERR_OK;
}
net_err_t udp_bind(sock_t *sock, const sock_addr_t *addr, sock_len_t len) {
    if (sock->local_ip.q_addr != 0) {
        return NET_ERR_BINDED;
    }
    ip_addr_t local_ip;
    sock_addr_in_t *in_addr = (sock_addr_in_t *) addr;
    ip_addr_from_buf(&local_ip, in_addr->in_addr.a_addr);
    int port = x_htons(in_addr->sin_port);

    udp_t *udp;
    nlist_node_t * node;
    nlist_for_each(node, &udp_list) {
        udp_t *get_udp = (udp_t *)nlist_entry(node, sock_t, node);
        if ((sock_t *) get_udp == sock) {
            continue;
        }
        if (udp->sock.local_ip.q_addr == local_ip.q_addr && udp->sock.local_port == port) {
            udp = get_udp;
            break;
        }
    }
    if (udp) {
        return NET_ERR_BINDED;
    } else {
        return sock_bind(sock, addr, len);
    }
    return NET_ERR_OK;
}

udp_t *udp_find(ip_addr_t *src_ip, uint16_t src_port, ip_addr_t *dest_ip, uint16_t dest_port) {
    nlist_node_t *node;
    nlist_for_each(node, &udp_list) {
        udp_t *udp = (udp_t *) nlist_entry(node, sock_t, node);
        if (udp->sock.local_ip.q_addr != 0 && udp->sock.local_ip.q_addr != dest_ip->q_addr) {
            continue;
        }
        if (dest_port != 0 && udp->sock.local_port != dest_port) {
            continue;
        }
        if (udp->sock.remote_ip.q_addr != 0 && udp->sock.remote_ip.q_addr != src_ip->q_addr) {
            continue;
        }
        if (udp->sock.remote_port != 0 && udp->sock.remote_port != src_port) {
            continue;
        }
        return udp;
    }
    return NULL;
}

net_err_t udp_connect(sock_t *sock, const sock_addr_t *addr, sock_len_t len) {

}

net_err_t udp_pkt_check(udp_pkt_t *pkt, int size) {
    return NET_ERR_OK;
}
net_err_t udp_in(pktbuf_t *buf, ip_addr_t *src, ip_addr_t *dest) {
    int iphdr_size = ipv4_hdr_size(&(((ipv4_pkt_t *)pktbuf_data(buf))->hdr));

    // 设置包的连续性，含IP包头和UDP包头
    net_err_t err = pktbuf_set_cont(buf, sizeof(udp_hdr_t) + iphdr_size);
    if (err < 0) {
        return err;
    }

    // 定位到UDP包
    ipv4_pkt_t * ip_pkt = (ipv4_pkt_t *)pktbuf_data(buf);
    udp_pkt_t* udp_pkt = (udp_pkt_t*)((uint8_t *)ip_pkt + iphdr_size);
    uint16_t local_port = x_ntohs(udp_pkt->hdr.dest_port);
    uint16_t remote_port = x_ntohs(udp_pkt->hdr.src_port);

    // 提交给合适的sock
    udp_t * udp = (udp_t*)udp_find(src, remote_port, dest, local_port);
    if (!udp) {
        return NET_ERR_UNREACH;
    }

    // 有UDP处理，移去IP包头，重新获得UDP包头
    pktbuf_remove_header(buf, iphdr_size);
    udp_pkt = (udp_pkt_t*)pktbuf_data(buf);
    if (udp_pkt->hdr.check_sum) {
        pktbuf_reset_acc(buf);
        if (checksum_peso(dest->a_addr, src->a_addr, NET_PROTOCOL_UDP, buf)) {
            return NET_ERR_CHECKSUM;
        }
    }

    // 检查包的合法性
    udp_pkt = (udp_pkt_t*)(pktbuf_data(buf));
    udp_pkt->hdr.src_port = x_ntohs(udp_pkt->hdr.src_port);
    udp_pkt->hdr.dest_port = x_ntohs(udp_pkt->hdr.dest_port);
    udp_pkt->hdr.total_len = x_ntohs(udp_pkt->hdr.total_len);
    if ((err = udp_pkt_check(udp_pkt, buf->total_size)) <  0) {
        return err;
    }
//    display_udp_packet(udp_pkt);

    // 填充来源信息
    pktbuf_remove_header(buf, (int)(sizeof(udp_hdr_t) - sizeof(udp_from_t)));
    udp_from_t* from = (udp_from_t *)pktbuf_data(buf);
    from->port = remote_port;
    ipaddr_copy(&from->from, src);

    // 写到接收队列，通知应用程序取
    if (nlist_count(&udp->recv_list) < UDP_MAX_RECV) {
        nlist_insert_last(&udp->recv_list, &buf->node);

        // 信号量通知线程去取包。如果需要由DNS处理，则将由DNS
//        if (dns_is_arrive(udp)) {
//            dns_in();
//        } else {
            sock_wakeup((sock_t *)udp, SOCK_WAIT_READ, NET_ERR_OK);
//        }
    } else {
        pktbuf_free(buf);
    }

    // DNS输入检查
    return NET_ERR_OK;
}

net_err_t udp_out(ip_addr_t *dest, uint16_t dport, ip_addr_t *src, uint16_t sport, pktbuf_t *buf) {

}

net_err_t
udp_send_to(sock_t *sock, const void *buf, size_t len, int falgs, const sock_addr_t *dest, sock_len_t dest_len,
            ssize_t *ret_len) {

}
net_err_t udp_receive_from(sock_t *sock, void *buf, size_t len, int flag, sock_addr_t *src, sock_len_t addr_len,
                           ssize_t *result_len) {
    udp_t * udp = (udp_t *)sock;

    // 取出一个数据包
    nlist_node_t * first = nlist_remove_first(&udp->recv_list);
    if (!first) {
        *result_len = 0;
        // 后续任务需要继续等待
        return NET_ERR_NEED_WAIT;
    }

    pktbuf_t* pktbuf = nlist_entry(first, pktbuf_t, node);

    // 填充IP地址与端口号地址来源信息
    udp_from_t* from = (udp_from_t *)pktbuf_data(pktbuf);
    sock_addr_in_t *addr = (sock_addr_in_t*)src;
    memset(addr, 0, sizeof(sock_addr_t));
    addr->sin_family = AF_INET;
    addr->sin_port = x_htons(from->port);     // 上层要大端
    ip_addr_to_buf(&from->from, addr->in_addr.a_addr);

    // 重新定位
    pktbuf_remove_header(pktbuf, sizeof(udp_from_t));       // TODO: 这是原来写了两个？

    // 从底层收到的是IP数据包，包含包头，方便读取各种信息
    int size = (pktbuf->total_size > (int)len) ? (int)len : pktbuf->total_size;
    pktbuf_reset_acc(pktbuf);
    net_err_t err = pktbuf_read(pktbuf, buf, size);
    if (err < 0) {
        pktbuf_free(pktbuf);
        return err;
    }
    log_debug("receive:%s\n", buf);


    pktbuf_free(pktbuf);

    if (result_len) {
        *result_len = (ssize_t)size;
    }
    return NET_ERR_OK;
}

net_err_t udp_send(sock_t *sock, const void *buf, size_t len, int flags, ssize_t *result_len) {
    return NET_ERR_OK;
}

net_err_t udp_receive(sock_t *sock, void *buf, size_t len, int flags, ssize_t *result_len) {
    sock_addr_t src;
    sock_len_t addr_len;
    return sock->ops->receive_from(sock, buf, len, flags, &src, addr_len, result_len);
}

sock_t *udp_create(int family, int protocol) {
    static sock_ops_t udp_ops = {
            .send_to = udp_send_to,
            .receive_from = udp_receive_from,
            .connect = udp_connect,
            .bind = udp_bind,
            .close = udp_close,
            .set_opt = udp_set_opt,
            .send = udp_send,
            .receive = udp_receive,
    };
    udp_t *udp = (udp_t *) mblock_alloc(&udp_mblock, 0);
    if (!udp) {
        return (sock_t *) NULL;
    }
    net_err_t err = sock_init((sock_t *) udp, family, protocol, &udp_ops);
    if (err < 0) {
        mblock_free(&udp_mblock, udp);
        return (sock_t *) NULL;
    }
    nlist_init(&udp->recv_list);
    udp->sock.recv_wait = &udp->recv_wait;
    if (sock_wait_init(udp->sock.recv_wait) < 0) {
        goto create_failed;
    }
    nlist_insert_last(&udp_list, &udp->sock.node);
    return (sock_t *) udp;
    create_failed:
    sock_uninit((sock_t *) udp);
    return (sock_t *) NULL;
}
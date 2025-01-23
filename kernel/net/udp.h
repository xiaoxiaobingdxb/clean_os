//
// Created by ByteDance on 2025/1/17.
//

#ifndef NET_UDP_H
#define NET_UDP_H

#include "net_err.h"
#include "sock.h"
#include "pktbuf.h"
#include "ip.h"

net_err_t init_udp();

sock_t *udp_create(int family, int protocol);

net_err_t udp_bind(sock_t *sock, const sock_addr_t *addr, sock_len_t len);

net_err_t udp_connect(sock_t *sock, const sock_addr_t *addr, sock_len_t len);

net_err_t udp_in(pktbuf_t *buf, ip_addr_t *src, ip_addr_t *dest);

net_err_t udp_out(ip_addr_t *dest, uint16_t dport, ip_addr_t *src, uint16_t sport, pktbuf_t *buf);

net_err_t udp_send(sock_t *sock, const void *buf, size_t len, int flags, ssize_t *result_len);

net_err_t udp_receive(sock_t *sock, void *buf, size_t len, int falgs, ssize_t *result_len);

net_err_t
udp_send_to(sock_t *sock, const void *buf, size_t len, int falgs, const sock_addr_t *dest, sock_len_t dest_len,
            ssize_t *ret_len);

net_err_t udp_receive_from(sock_t *sock, void *buf, size_t len, int flag, sock_addr_t *src, sock_len_t addr_len,
                           ssize_t *ret_len);

#endif //NET_UDP_H

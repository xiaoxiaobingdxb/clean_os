//
// Created by ByteDance on 2025/1/22.
//

#ifndef NET_SOCKET_H
#define NET_SOCKET_H
#include "include/fs_model.h"
#include "sock.h"
#include "include/socket.h"

fd_t create_socket(int family, int type, int protocol);
socket_t *get_socket(fd_t fd);
int close_socket(fd_t sock_fd);
int socket_bind(fd_t sock_fd, const sock_addr_t *addr, sock_len_t len);
int socket_listen(fd_t sock_fd, int backlog);
int socket_accept(fd_t sock_fd, sock_addr_t *addr, sock_len_t *len);
int socket_connect(fd_t sock_fd, sock_addr_t *addr, sock_len_t len);
ssize_t socket_send_to(fd_t sock_fd, const void *buf, size_t len, int flags, const sock_addr_t *dest, sock_len_t dest_len);
ssize_t socket_receive_from(fd_t sock_fd, void *buf, size_t len, int flags, sock_addr_t *src, sock_len_t *src_len);
ssize_t socket_send(fd_t sock_fd, const void *buf, size_t len, int flags);
ssize_t socket_receive(fd_t sock_fd, void *buf, size_t len, int flags);
int set_sock_opt(fd_t sock_fd, int level, int opt_name, const char *opt_value, int opt_len);
#endif //NET_SOCKET_H

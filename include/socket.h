//
// Created by ByteDance on 2025/1/22.
//

#ifndef SOCKET_H
#define SOCKET_H

#include "common/types/basic.h"
#include "fs_model.h"
#include "net_util.h"

#undef AF_INET
#define AF_INET                 0               // IPv4协议簇

#undef SOCK_RAW
#define SOCK_RAW                1               // 原始数据报
#undef SOCK_DGRAM
#define SOCK_DGRAM              2               // 数据报式套接字
#undef SOCK_STREAM
#define SOCK_STREAM             3               // 流式套接字

#undef IPPROTO_ICMP
#define IPPROTO_ICMP            1               // ICMP协议
#undef IPPROTO_UDP
#define IPPROTO_UDP             17              // UDP协议

#undef IPPROTO_TCP
#define IPPROTO_TCP             6               // TCP协议

typedef struct {
    uint8_t sa_len;
    uint8_t sa_family;
    uint8_t sa_data[14];
} sock_addr_t;

typedef struct {
    uint8_t sin_len;
    uint8_t sin_family;
    uint16_t sin_port;
    union {
        uint32_t q_addr;
        uint8_t a_addr[4];
    } in_addr;
    char sin_zero[8];
} sock_addr_in_t;


typedef int sock_len_t;

fd_t socket(int family, int type, int protocol);
int bind(fd_t sock_fd, const sock_addr_t *addr, sock_len_t addr_len);
int receive(fd_t sock_fd, void *buf, size_t len, int flags, ssize_t *result_len);
int connect(fd_t sock_fd, const sock_addr_t *addr, sock_len_t addr_len);
ssize_t send(fd_t sock_fd, const void *buf, size_t len, int flags);

#endif //SOCKET_H

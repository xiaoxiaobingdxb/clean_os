//
// Created by ByteDance on 2025/1/16.
//

#ifndef NET_RAW_H
#define NET_RAW_H

#include "pktbuf.h"
#include "net_err.h"
#include "sock.h"

net_err_t init_raw();
sock_t *raw_create(int family, int protocol);
net_err_t raw_in(pktbuf_t *buf);

#endif //NET_RAW_H

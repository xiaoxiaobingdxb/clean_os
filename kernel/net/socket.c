//
// Created by ByteDance on 2025/1/22.
//

#include "socket.h"
#include "exmsg.h"

fd_t create_socket(int family, int type, int protocol) {
    sock_req_t req;
    req.wait = 0;
    req.sock_fd = -1;
    req.create.family = family;
    req.create.type = type;
    req.create.protocol = protocol;
    net_err_t err = exmsg_func_exec(handle_sock_create, &req);
    if (err < 0) {
        return -1;
    }
    return req.sock_fd;
}

int close_socket(fd_t sock_fd) {
    return 0;
}

int socket_bind(fd_t sock_fd, const sock_addr_t *addr, sock_len_t len) {
    if (!addr || len != sizeof(sock_addr_t)) {
        return -1;
    }
    if (addr->sa_family != AF_INET) {
        return -1;
    }
    sock_req_t req;
    req.wait = 0;
    req.sock_fd = sock_fd;
    req.bind.addr = addr;
    req.bind.len = len;
    net_err_t err = exmsg_func_exec(handle_sock_bind, &req);
    if (err < 0) {
        return -1;
    }
    return 0;
}

int socket_listen(fd_t sock_fd, int backlog) {
    return 0;
}

int socket_accept(fd_t sock_fd, sock_addr_t *addr, sock_len_t *len) {
    return 0;
}

ssize_t
socket_send_to(fd_t sock_fd, const void *buf, size_t len, int flags, const sock_addr_t *dest, sock_len_t dest_len) {
    if (len == 0 || dest_len != sizeof(sock_addr_t)) {
        return -1;
    }
    if (dest->sa_family != AF_INET) {
        return -1;
    }
    ssize_t send_size = 0;
    uint8_t * start = (uint8_t * )
    buf;
    while (len) {
        sock_req_t req;
        req.wait = 0;
        req.sock_fd = sock_fd;
        req.data.buf = start;
        req.data.len = len;
        req.data.flags = flags;
        req.data.addr = (sock_addr_t*)dest;
        req.data.addr_len = &dest_len;
        req.data.comp_len = 0;
        net_err_t err = exmsg_func_exec(handle_sock_send, &req);
        if (err < 0) {
            return -1;
        }
        if (req.wait && (err = sock_wait_enter(req.wait, req.wait_tmo)) < 0) {
            return -1;
        }
        len -= req.data.comp_len;
        send_size += (ssize_t) req.data.comp_len;
        start += req.data.comp_len;
    }
    return send_size;
}

ssize_t socket_receive_from(fd_t sock_fd, void *buf, size_t len, int flags, sock_addr_t *src, sock_len_t *src_len) {
    if (len == 0 || !src_len || !src) {
        return -1;
    }
    while (true) {
        sock_req_t req;
        req.wait = 0;
        req.sock_fd = sock_fd;
        req.data.buf = buf;
        req.data.len = len;
        req.data.flags = flags;
        req.data.comp_len = 0;
        req.data.addr = src;
        req.data.addr_len = src_len;
        net_err_t err = exmsg_func_exec(handle_sock_receive, &req);
        if (err < 0) {
            return -1;
        }
        if (req.data.comp_len) {
            return (ssize_t) req.data.comp_len;
        }
        err = sock_wait_enter(req.wait, req.wait_tmo);
        if (err == NET_ERR_CLOSE) {
            return NET_ERR_EOF;
        }
        if (err < 0) {
            return -1;
        }

    }
}

ssize_t socket_send(fd_t sock_fd, const void *buf, size_t len, int flags) {
    ssize_t send_size = 0;
    uint8_t * start = (uint8_t * )
    buf;
    while (len) {
        sock_req_t req;
        req.wait = 0;
        req.sock_fd = sock_fd;
        req.data.buf = start;
        req.data.len = len;
        req.data.flags = flags;
        req.data.comp_len = 0;
        net_err_t err = exmsg_func_exec(handle_sock_send, &req);
        if (err < 0) {
            return -1;
        }
        if (req.wait && (err = sock_wait_enter(req.wait, req.wait_tmo)) < 0) {
            return -1;
        }
        len -= req.data.comp_len;
        send_size += (ssize_t) req.data.comp_len;
        start += req.data.comp_len;
    }
    return send_size;
}

ssize_t socket_receive(fd_t sock_fd, void *buf, size_t len, int flags) {
    if (len == 0) {
        return -1;
    }
    while (true) {
        sock_req_t req;
        req.wait = 0;
        req.sock_fd = sock_fd;
        req.data.buf = buf;
        req.data.len = len;
        req.data.flags = flags;
        req.data.comp_len = 0;
        net_err_t err = exmsg_func_exec(handle_sock_receive, &req);
        if (err < 0) {
            return -1;
        }
        if (req.data.comp_len) {
            return (ssize_t) req.data.comp_len;
        }
        err = sock_wait_enter(req.wait, req.wait_tmo);
        if (err == NET_ERR_CLOSE) {
            return NET_ERR_EOF;
        }
        if (err < 0) {
            return -1;
        }
    }
}

int set_sock_opt(fd_t sock_fd, int level, int opt_name, const char *opt_value, int opt_len) {
    if (!opt_value || !opt_len) {
        return -1;
    }
    sock_req_t req;
    req.wait = 0;
    req.sock_fd = sock_fd;
    req.opt.level = level;
    req.opt.opt_name = opt_len;
    req.opt.opt_value = opt_value;
    req.opt.opt_len = opt_len;
    net_err_t  err = exmsg_func_exec(handle_sock_set_opt, &req);
    if (err < 0) {
        return err;
    }
    return 0;
}
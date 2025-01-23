//
// Created by ByteDance on 2025/1/22.
//

#include "include/socket.h"
#include "include/syscall_no.h"
#include "base.h"

fd_t socket(int family, int type, int protocol) {
    return syscall3(SYSCALL_socket, family, type, protocol);
}
int bind(fd_t sock_fd, const sock_addr_t *addr, sock_len_t len) {
    return syscall3(SYSCALL_bind, sock_fd, addr, len);
}

int receive(fd_t sock_fd, void *buf, size_t len, int flags, ssize_t *result_len) {
    return syscall4(SYSCALL_receive, sock_fd, buf, len, flags);
}
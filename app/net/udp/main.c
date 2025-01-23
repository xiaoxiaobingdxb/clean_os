#include "common/tool/lib.h"
#include "include/syscall.h"
#include "glibc/io/std.h"
#include "glibc/memory/malloc//malloc.h"
#include "common/lib/string.h"

void udp_process() {
    fd_t sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0) {
        printf("create_socket fail %d\n", sock_fd);
        return;
    }
    static sock_addr_in_t addr = {
            .sin_family = AF_INET,
            .sin_port = 8000,
    };
    addr.in_addr.a_addr[0] = 10;
    addr.in_addr.a_addr[1] = 3;
    addr.in_addr.a_addr[2] = 208;
    addr.in_addr.a_addr[3] = 99;
    int err = 0;
    if ((err = bind(sock_fd, (sock_addr_t * ) & addr, sizeof(addr))) < 0) {
        printf("bind_socket fail %d\n", err);
        return;
    }
    while (true) {
        void *buf = malloc(10);
        memset(buf, 0, 10);
        receive(sock_fd, buf, 4, 0, NULL);
        printf("receive:%s\n", buf);
        yield();
    }
}

int main(int argc, char **argv) {
    udp_process();
    return 0;
}
#include "include/syscall.h"
#include "glibc/io/std.h"

void ping(const char *ip, int times) {
    for (int i = 0; i < times; i++) {
        timespec_t time;
        timestamp(&time);
        datetime_t datetime;
        mkdatetime(time.tv_sec, &datetime);
        printf("%d sleep_test at %d:%d\n", i, datetime.minute, datetime.second);
        sleep(1000 * 3);
    }
    return;
    fd_t sock_fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock_fd < 0) {
        printf("create_socket fail %d\n", sock_fd);
        return;
    }

    sock_addr_in_t addr;
    addr.sin_family = AF_INET; //地址规格
    addr.sin_port = 0;          // 端口未用
    addr.in_addr.q_addr = inet_addr(ip);
    if (connect(sock_fd, (sock_addr_t * ) & addr, sizeof(addr))) {
        printf("connect_sock fail\n");
        return;
    }
}
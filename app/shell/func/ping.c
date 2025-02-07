#include "include/syscall.h"
#include "glibc/io/std.h"

#pragma pack(1)
typedef struct {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint16_t echo_id;
    uint16_t echo_seq;
} icmp_hdr_t;
typedef struct {
    icmp_hdr_t icmp_hdr;
    uint8_t buf[256];
} ping_req_t;

typedef struct {
    union {
        struct {
            uint16_t shdr: 4;
            uint16_t version: 4;
            uint16_t ds: 6;
            uint16_t ecn: 2;
        };
        uint16_t shdr_all;
    };
    uint16_t total_len;
    uint16_t id;
    union {
        struct {
            uint16_t offset: 13;
            uint16_t more: 1;
            uint16_t disable: 1;
            uint16_t resvered: 1;
        };
        uint16_t frag_all;
    };
    uint8_t ttl;
    uint8_t protocol;
    uint16_t hdr_checksum;
    uint8_t src_ip[4];
    uint8_t dest_ip[4];

} ipv4_hdr_t;
typedef struct {
    ipv4_hdr_t ip_hdr;
    icmp_hdr_t icmp_hdr;
    uint8_t buf[256];;
} ping_resp_t;
#pragma pack()

static uint16_t checksum16(uint32_t offset, void *buf, uint16_t len, uint32_t pre_sum, int complement) {
    uint16_t *curr_buf = (uint16_t *) buf;
    uint32_t checksum = pre_sum;

    // 起始字节不对齐, 加到高8位
    if (offset & 0x1) {
        uint8_t *buf = (uint8_t *) curr_buf;
        checksum += *buf++ << 8;
        curr_buf = (uint16_t *) buf;
        len--;
    }

    while (len > 1) {
        checksum += *curr_buf++;
        len -= 2;
    }

    if (len > 0) {
        checksum += *(uint8_t *) curr_buf;
    }

    // 注意，这里要不断累加。不然结果在某些情况下计算不正确
    uint16_t high;
    while ((high = checksum >> 16) != 0) {
        checksum = high + (checksum & 0xffff);
    }

    return complement ? (uint16_t)~checksum : (uint16_t) checksum;
}

void ping(const char *ip, size_t count, size_t size) {
    for (int i = 0; i < count; i++) {
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
        printf("PING %s(%s): %d data bytes\n", ip, ip, size);
        ping_req_t req;
        req.icmp_hdr.type = 8;
        req.icmp_hdr.code = 0;
        req.icmp_hdr.checksum = 0;

        ping_resp_t resp;
        ssize_t receive_size;

        timespec_t start_time, end_time;
        req.icmp_hdr.echo_id = 0;

        req.icmp_hdr.echo_seq = i;
        req.icmp_hdr.checksum = checksum16(0, &req, sizeof(icmp_hdr_t) + size, 0, 1);
        timestamp(&start_time);
        ssize_t send_size = send(sock_fd, &req, sizeof(icmp_hdr_t) + size, 0);
        if (send_size <= 0) {
            printf("send_size=%d\n", send_size);
            break;
        }

        int ret = receive(sock_fd, &resp, sizeof(resp), 0, &receive_size);
        timestamp(&end_time);
        uint64_t start_ms = timespec2ms(&start_time);
        uint64_t end_ms = timespec2ms(&end_time);
        printf("%d bytes from %s: icmp_seq=%d ttl=%d time=%d ms\n", ret, ip, resp.icmp_hdr.echo_seq, 64,
               (uint32_t)(end_ms - start_ms));
//        printf("ret=%d,receive_size=%d\n", ret, receive_size);
        /*timespec_t time;
        timestamp(&time);
        datetime_t datetime;
        mkdatetime(time.tv_sec, &datetime);
        printf("%d sleep_test at %d:%d\n", i, datetime.minute, datetime.second);
         */
        close(sock_fd);
        sleep(1000 * 1);
    }
}
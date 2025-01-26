#if !defined(NET_UTIL_H)
#define NET_UTIL_H
#include "common/lib/stdio.h"
#include "common/lib/string.h"
#include "common/types/basic.h"
#include "protocol.h"
#include "pktbuf.h"
#include "include/net_util.h"

#define x_htons(v)       htons(v)
#define  x_ntohs(v)      ntohs(v)

static uint16_t checksum16(uint32_t offset, void* buf, uint16_t len, uint32_t pre_sum, int complement) {
    uint16_t* curr_buf = (uint16_t *)buf;
    uint32_t checksum = pre_sum;

    // 起始字节不对齐, 加到高8位
    if (offset & 0x1) {
        uint8_t * buf = (uint8_t *)curr_buf;
        checksum += *buf++ << 8;
        curr_buf = (uint16_t *)buf;
        len--;
    }

    while (len > 1) {
        checksum += *curr_buf++;
        len -= 2;
    }

    if (len > 0) {
        checksum += *(uint8_t*)curr_buf;
    }

    // 注意，这里要不断累加。不然结果在某些情况下计算不正确
    uint16_t high;
    while ((high = checksum >> 16) != 0) {
        checksum = high + (checksum & 0xffff);
    }

    return complement ? (uint16_t)~checksum : (uint16_t)checksum;
}

static uint16_t checksum_peso(const uint8_t * src_ip, const uint8_t* dest_ip, uint8_t protocol, pktbuf_t * buf) {
    uint8_t zero_protocol[2] = { 0, protocol };
    uint16_t len = x_htons(buf->total_size);

    int offset = 0;
    uint32_t sum = checksum16(offset, (uint16_t*)src_ip, IPV4_ADDR_SIZE, 0, 0);
    offset += IPV4_ADDR_SIZE;
    sum = checksum16(offset, (uint16_t*)dest_ip, IPV4_ADDR_SIZE, sum, 0);
    offset += IPV4_ADDR_SIZE;
    sum = checksum16(offset, (uint16_t*)zero_protocol, 2, sum, 0);
    offset += 2;
    sum = checksum16(offset, (uint16_t*)&len, 2, sum, 0);

    pktbuf_reset_acc(buf);
    sum = pktbuf_checksum16(buf, buf->total_size, sum, 1);
    return sum;
}

#endif // NET_UTIL_H

#include "icmp.h"
#include "ip.h"
#include "raw.h"
#include "ether.h"

#pragma pack(1)
// @ref https://wiki.osdev.org/Internet_Control_Message_Protocol
typedef struct {
    uint8_t type;
    uint8_t code;
    uint8_t checksum;
} icmp_hdr_t;
typedef struct {
    icmp_hdr_t hdr;
    union {
        uint32_t reverse;
    };
    uint8_t data[1];
} icmp_pkt_t;
#pragma pack(0)

enum {
    ICMP_ECHO_REPLY = 0,                  // 回送响应
    ICMP_ECHO_REQUEST = 8,                // 回送请求
    ICMP_UNREACH = 3,                     // 不可达
};

net_err_t icmp_pkt_check(icmp_hdr_t *hdr, int size, pktbuf_t *buf) {
    if (size <= sizeof(icmp_hdr_t)) {
        return NET_ERR_SIZE;
    }
    uint16_t checksum = pktbuf_checksum16(buf, size, 0, 1);
    if (checksum != 0) {
        return NET_ERR_CHECKSUM;
    }
    return NET_ERR_OK;
}

net_err_t icmp_echo_reply(ip_addr_t *src, ip_addr_t *dest, pktbuf_t *buf) {
    icmp_pkt_t *pkt = (icmp_pkt_t *) pktbuf_data(buf);
    pkt->hdr.type = ICMP_ECHO_REPLY;
    pkt->hdr.checksum = 0;
    return icmp_out(dest, src, buf);
}

net_err_t icmp_in(ip_addr_t *src, ip_addr_t *dest, pktbuf_t *buf) {
    ipv4_pkt_t *ip_pkt = (ipv4_pkt_t *) pktbuf_data(buf);
    int hdr_size = ipv4_hdr_size(&ip_pkt->hdr);

    net_err_t err = pktbuf_set_cont(buf, sizeof(icmp_hdr_t) + hdr_size);
    if (err < 0) {
        return err;
    }
    ip_pkt = (ipv4_pkt_t *) pktbuf_data(buf);
    icmp_pkt_t *pkt = (icmp_pkt_t *) (pktbuf_data(buf) + hdr_size);
    pktbuf_reset_acc(buf);
    pktbuf_seek(buf, hdr_size);
    if ((err = icmp_pkt_check(&pkt->hdr, buf->total_size - hdr_size, buf)) < 0) {
        return err;
    }
    if (pkt->hdr.type == ICMP_ECHO_REQUEST) {
        err = pktbuf_remove_header(buf, hdr_size);
        if (err < 0) {
            return err;
        }
        return icmp_echo_reply(src, dest, buf);
    } else {
        return raw_in(buf);
    }
    return NET_ERR_OK;
}

net_err_t icmp_out(ip_addr_t *src, ip_addr_t *dest, pktbuf_t *buf) {
    icmp_pkt_t *pkt = (icmp_pkt_t *) pktbuf_data(buf);
    pktbuf_seek(buf, 0);
    pkt->hdr.checksum = pktbuf_checksum16(buf, buf->total_size, 0, 1);
    ipv4_out(NET_PROTOCOL_ICMPv4, dest, src, buf);
}
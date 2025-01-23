//
// Created by ByteDance on 2025/1/21.
//

#ifndef NET_PROTOCOL_H
#define NET_PROTOCOL_H

#define ETH_HWA_SIZE 6
#define IPV4_ADDR_SIZE 4
#define ARP_HW_ETHER 0x1  // 以太网类型
#define ARP_REQUEST 0x1   // ARP请求包
#define ARP_REPLY 0x2     // ARP响应包
/**
 * 常见协议类型
 */
typedef enum _protocol_t {
    NET_PROTOCOL_ARP = 0x0806,   // ARP协议
    NET_PROTOCOL_IPv4 = 0x0800,  // IP协议
    NET_PROTOCOL_ICMPv4 = 0x1,   // ICMP协议
    NET_PROTOCOL_UDP = 0x11,     // UDP协议
    NET_PROTOCOL_TCP = 0x06,     // TCP协议
} protocol_t;

#endif //NET_PROTOCOL_H

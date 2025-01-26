//
// Created by ByteDance on 2025/1/26.
//

#ifndef INCLUDE_NET_UTIL_H
#define INCLUDE_NET_UTIL_H

#include "common/types/basic.h"
#include "common/lib/string.h"
#include "common/lib/stdio.h"
static void mac2str(uint8_t mac[6], const char *str) {
    memset(str, '\0', strlen(str));
    char *buf = "aa:";
    for (int i = 0; i < 5; i++) {
        if (mac[i] == 0) {
            buf = "00:";
        } else {
            sprintf(buf, "%x:", mac[i]);
        }
        memcpy(str + i * 3, buf, 3);
        if (i == 4) {
            if (mac[5] == 0) {
                buf = "00:";
            } else {
                sprintf(buf, "%x", mac[5]);
            }
            memcpy(str + 5 * 3, buf, 2);
        }
    }
}

static void ip2str(uint8_t ip[4], char *str) {
    memset(str, '\0', strlen(str));
    sprintf(str, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
}

static uint32_t str2ip(const char *ip_str) {
    char str[3*4+3];
    memset(str, 0, sizeof(str) / sizeof(char));
    memcpy(str, ip_str, strlen(ip_str));
    union {
        uint8_t a_addr[4];
        uint32_t q_addr;
    } addr;
    char *token;
    const char *delim = ".";
    token = strtok(str, delim);
    int idx = 0;
    while (token != NULL && idx < 4) {
        int num;
        str2num(token, &num);
        addr.a_addr[idx] = (uint8_t)num;
        token = strtok(NULL, delim);
        idx++;
    }
    return addr.q_addr;
}

static inline uint16_t swap_u16(uint16_t v) {
    // 7..0 => 15..8, 15..8 => 7..0
    uint16_t r = ((v & 0xFF) << 8) | ((v >> 8) & 0xFF);
    return r;
}

#define inet_addr(x) str2ip(x)
#define ntohs(x)      swap_u16(x)
#define htons(x)      swap_u16(x)


#endif //INCLUDE_NET_UTIL_H

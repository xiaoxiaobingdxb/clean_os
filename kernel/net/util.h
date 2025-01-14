#if !defined(NET_UTIL_H)
#define NET_UTIL_H
#include "common/lib/stdio.h"
#include "common/lib/string.h"
#include "common/types/basic.h"
static void mac2str(uint8_t mac[6], const char* str) {
    memset(str, '\0', strlen(str));
    char* buf = "aa:";
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

static inline uint16_t swap_u16(uint16_t v) {
    // 7..0 => 15..8, 15..8 => 7..0
    uint16_t r = ((v & 0xFF) << 8) | ((v >> 8) & 0xFF);
    return r;
}
#endif // NET_UTIL_H

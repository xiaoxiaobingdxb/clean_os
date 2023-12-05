#include "string.h"

#include "../tool/math.h"
const void *memset(const void *dst, uint8_t c, size_t len) {
    const unsigned char uc = c;
    unsigned char *su;
    for (su = (unsigned char *)dst; 0 < len; ++su, --len) {
        *su = uc;
    }
    return dst;
}

void memcpy(const void *dst, const void *src, size_t size) {
    uint8_t *dst_ = (uint8_t *)dst;
    const uint8_t *src_ = src;
    while (size-- > 0) {
        *dst_++ = *src_++;
    }
}

const char *strcat(const char *dst, const char *src) {
    char *str = (char *)dst;
    char *src_ = (char *)src;
    while (*str++)
        ;
    --str;
    while ((*str++ = *src_++))
        ;
    return dst;
}

int memcmp(const void *a, const void *b, size_t size) {
    const uint8_t *a_ = a;
    const uint8_t *b_ = b;
    while (size-- > 0) {
        if (*a_ != *b_) {
            return *a_ > *b_ ? 1 : -1;
        }
        a_++;
        b_++;
    }
    return 0;
}

size_t strlen(const char *str) {
    const char *p = str;
    while (*p++)
        ;
    return (p - str - 1);
}

size_t replace(const char *str, char new, char old) {
    size_t count = 0;
    for (int i = 0; i < strlen(str); i++) {
        if (str[i] == old) {
            *((char*)str + i) = new;
            count++;
        }
    }
    return count;
}

size_t trim(const char *str) {
    return replace(str, 0, ' ');
}

size_t trim_strlen(const char *str) {
    const char *p = str;
    while (p > 0 && *p != ' ') {
        p++;
    }
    return p - str;
}

char *strchr(const char *str, const char ch) {
    while (*str != 0) {
        if (*str == ch) {
            return (char *)str;
        }
        str++;
    }
    return NULL;
}
char *strrchr(const char *str, const char ch) {
    const char *last = NULL;
    while (*str != 0) {
        if (*str == ch) {
            last = str;
        }
        str++;
    }
    return (char *)last;
}

uint32_t strchrs(const char *str, const char ch) {
    uint32_t ch_cnt = 0;
    const char *p = str;
    while (*p != 0) {
        if (*p == ch) {
            ch_cnt++;
        }
    }
    return ch_cnt;
}

int str2num(const char *str, int *num) {
    size_t size = strlen(str);
    if (size < 1) {
        return -1;
    }
    int signe = 1;
    if (str[0] == '-') {
        signe = -1;
        str++;
        size--;
    }
    int ret = 0;
    for (int i = size - 1, j = 0; i >= 0; i--, j++) {
        char ch = str[i];
        if (ch < '0' || ch > '9') {
            return -1;
        }
        int n = str[i] - '0';
        ret += n * pow(10, j);
    }
    if (num != NULL) {
        *num = signe * ret;
    }
    return 0;
}
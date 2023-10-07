#include "string.h"
const void *memset(const void *dst, uint8_t c, size_t len) {
    const unsigned char uc = c;
    unsigned char *su;
    for (su = (unsigned char*)dst; 0 < len; ++su, --len) {
        *su = uc;
    }
    return dst;
}

void memcpy(const void *dst, const void *src, size_t size) {
    uint8_t *dst_ = (uint8_t*)dst;
    const uint8_t *src_ = src;
    while (size-- > 0) {
        *dst_++ = *src_++;
    }
}

const char *strcat(const char *dst, const char *src) {
    char *str = (char*)dst;
    char *src_ = (char*)src;
    while (*str++);
    --str;
    while ((*str++ = *src_++));
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

uint32_t strlen(const char *str) {
    const char *p = str;
    while (*p++)
        ;
    return (p - str - 1);
}

char *strchr(const char *str, const char ch) {
    while (*str != 0) {
        if (*str == ch) {
            return (char *)str;
        }
        str++;
    }
}
char *strrchr(const char *str, const char ch) {
    const char *last = NULL;
    while (*str != 0) {
        if (*str == ch) {
            last = str;
        }
        str++;
    }
    return (char*)last;
}

uint32_t strchrs(const char* str, const char ch) {
    uint32_t ch_cnt = 0;
    const char *p = str;
    while (*p != 0) {
        if (*p == ch) {
            ch_cnt++;
        }
    }
    return ch_cnt;
}
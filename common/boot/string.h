#ifndef BOOT_STRING_H
#define BOOT_STRING_H
#include "../types/basic.h"
static inline void *memset(void *dst, int c, size_t len)
{
    const unsigned char uc = c;
    unsigned char *su;
    for (su = dst; 0 < len; ++su, --len)
    {
        *su = uc;
    }
    return dst;
}

#define NULL ((void *)0)
#endif
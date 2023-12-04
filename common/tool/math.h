#ifndef TOOL_MATH_H
#define TOOL_MATH_H

#include "common/types/basic.h"

static inline uint32_t down2(uint32_t origin, uint32_t bound) {
    return origin - origin % bound;
}

static inline uint32_t up2(uint32_t origin, uint32_t bound) {
    if (origin % bound == 0) {
        return origin;
    }
    return down2(origin, bound) + bound;
}

static inline uint32_t pow(uint32_t x, uint32_t y) {
    uint32_t ret = 1;
    for (uint32_t i = 0; i < y; i++) {
        ret *= x;
    }
    return ret;
}

#define min(a, b) (a < b ? a : b)
#define max(a, b) (a > b ? a : b)
#endif
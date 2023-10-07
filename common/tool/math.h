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

#endif
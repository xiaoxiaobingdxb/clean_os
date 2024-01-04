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

#define do_div(n, base)                                              \
    ({                                                               \
        unsigned long __upper, __low, __high, __mod, __base;         \
        __base = (base);                                             \
        if (__builtin_constant_p(__base) && is_power_of_2(__base)) { \
            __mod = n & (__base - 1);                                \
            n >>= ilog2(__base);                                     \
        } else {                                                     \
            asm("" : "=a"(__low), "=d"(__high) : "A"(n));            \
            __upper = __high;                                        \
            if (__high) {                                            \
                __upper = __high % (__base);                         \
                __high = __high / (__base);                          \
            }                                                        \
            asm("divl %2"                                            \
                : "=a"(__low), "=d"(__mod)                           \
                : "rm"(__base), "0"(__low), "1"(__upper));           \
            asm("" : "=A"(n) : "a"(__low), "d"(__high));             \
        }                                                            \
        __mod;                                                       \
    })

static inline uint64_t div_u64_rem(uint64_t dividend, uint32_t divisor,
                                   uint32_t *remainder) {
    union {
        uint64_t v64;
        uint32_t v32[2];
    } d = {dividend};
    uint32_t upper;

    upper = d.v32[1];
    d.v32[1] = 0;
    if (upper >= divisor) {
        d.v32[1] = upper / divisor;
        upper %= divisor;
    }
    __asm__("divl %2"
        : "=a"(d.v32[0]), "=d"(*remainder)
        : "rm"(divisor), "0"(d.v32[0]), "1"(upper));
    return d.v64;
}
#define div_u64_rem div_u64_rem

static inline uint64_t mul_u32_u32(uint32_t a, uint32_t b) {
    uint32_t high, low;

    __asm__("mull %[b]" : "=a"(low), "=d"(high) : [a] "a"(a), [b] "rm"(b));

    return low | ((uint64_t)high) << 32;
}
#endif
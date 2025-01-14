#if !defined(CPU_MIO_H)
#define CPU_MIO_H
#include "../types/basic.h"
static inline void moutl(uint32_t addr, uint32_t value) {
    *((volatile uint32_t *)addr) = value;
}
static inline uint32_t minl(uint32_t addr) {
    return *((volatile uint32_t *)addr);
}

#endif // CPU_MIO_H


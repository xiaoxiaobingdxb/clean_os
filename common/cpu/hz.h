#ifndef PU_HZ_H
#define PU_HZ_H

#include "../types/basic.h"

typedef struct {
    uint32_t base_hz;
    uint32_t max_hz;
    uint32_t bus_hz;
} cpu_hz_t;
static void get_hz(cpu_hz_t *hz) {
    unsigned int eax, ebx, ecx, edx;
//使用CPUID指令,功能号0x0用于获取CPU频率信息
    eax = 0x16;
    __asm__ __volatile__ (
            "cpuid"
            : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx)
            : "0" (eax)
            );
// eax: Processor Base Frequency (in MHz)
// ebx: Maximum Frequency (in MHz)
// ecx: Bus (Reference) Frequency (in MHz)
    hz->base_hz = eax;
    hz->max_hz = ebx;
    hz->bus_hz = ecx;
}


#endif //PU_HZ_H

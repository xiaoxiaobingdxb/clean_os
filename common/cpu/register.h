#ifndef CPU_REGISTER_H
#define CPU_REGISTER_H
#include "../types/basic.h"
static inline uint16_t ds()
{
	uint16_t seg;
	__asm__ __volatile__("movw %%ds,%[v]"
						 : [v] "=r"(seg));
	return seg;
}
static inline uint16_t fs()
{
	uint16_t seg;
	__asm__ __volatile__("movw %%fs,%[v]"
						 : [v] "=r"(seg));
	return seg;
}
static inline uint16_t gs()
{
	uint16_t seg;
	__asm__ __volatile__("movw %%gs,%[v]"
						 : [v] "=r"(seg));
	return seg;
}
#endif
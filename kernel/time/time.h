#include "include/syscall.h"

#define HZ 1000

void sys_timestamp(timespec_t *timestamp);

void jiffies2time(uint64_t jiffies, timespec_t *value);
#include "time.h"

#include "../device/rtc/cmos.h"
#include "common/tool/datetime.h"
#include "common/tool/math.h"
#include "pit.h"

void sys_timestamp(timespec_t *timestamp) {
    if (timestamp == NULL) {
        return;
    }
    time_t time;
    get_rtc_time(&time);
    timestamp->tv_sec = mktime64(time.year, time.month, time.day, time.hour,
                                 time.minute, time.second);
    uint64_t jiffies = get_jiffies();
    timespec_t spec;
    jiffies2time(jiffies, &spec);
    timestamp->tv_nsec = spec.tv_nsec;
}

void jiffies2time(uint64_t jiffies, timespec_t *value) {
    uint32_t remain;
    value->tv_sec = div_u64_rem(jiffies, HZ, &remain);
    value->tv_nsec = mul_u32_u32(remain, sec2ns(1) / HZ);
}
#if !defined(TOOL_DATETIME_H)
#define TOOL_DATETIME_H

#include "../types/basic.h"
#include "math.h"

#define sec2ms(n) (n * 1000)
#define ms2us(n) (n * 1000)
#define us2ns(n) (n * 1000)
#define ms2ns(n) (us2ns(ms2us(n)))
#define sec2us(n) (ms2us(sec2ms(n)))
#define sec2ns(n) (ms2ns(sec2ms(n)))

#define minu2sec(n) (n * 60)
#define hour2minu(n) (n * 60)
#define hour2sec(n) (minu2sec(hour2min(n)))
#define day2hour(n) (n * 24)
#define day2minu(n) (hour2minu(day2hour(n)))
#define day2sec(n) (minu2sec(day2minu(n)))

typedef sint64_t time64_t;

typedef struct {
    time64_t tv_sec;
    time64_t tv_nsec;
} timespec_t;

typedef struct {
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
} datetime_t;

static inline time64_t mktime64(const unsigned int year0, const unsigned int mon0,
		const unsigned int day, const unsigned int hour,
		const unsigned int min, const unsigned int sec)
{
	unsigned int mon = mon0, year = year0;

	/* 1..12 -> 11,12,1..10 */
	if (0 >= (int) (mon -= 2)) {
		mon += 12;	/* Puts Feb last since it has leap day */
		year -= 1;
	}

	time64_t time= ((((time64_t)
		  (year/4 - year/100 + year/400 + 367*mon/12 + day) +
		  year*365 - 719499
	    )*24 + hour /* now have hours - midnight tomorrow handled here */
	  )*60 + min /* now have minutes */
	)*60 + sec; /* finally seconds */
    return time;
}


static inline void mkdatetime(time64_t time, datetime_t *datetime) {
    int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (!datetime) {
        return;
    }
    time = div_u64_rem(time, 60, &datetime->second);
    time = div_u64_rem(time, 60, &datetime->minute);
    int year4 = div_u64_rem(time, 1461 * 24, NULL);
    datetime->year = 1970 + year4 * 4;
    uint32_t remainder;
    div_u64_rem(time, 1461 * 24, &remainder);
    time = remainder;
    for(;;) {
        int hours_per_year = 365 * 24;
        if ((datetime->year & 3) == 0) {
            hours_per_year += 24;
        }
        if (time < hours_per_year) {
            break;
        }
        datetime->year++;
        time -= hours_per_year;
    }
    time = div_u64_rem(time, 24, &datetime->hour);
    time++;
    if ((datetime->year & 3) == 0) {
        if (time > 60) {
            time--;
        } else if (time == 60) {
            datetime->month = 1;
            datetime->day = 29;
            return;
        }
    }
    for (datetime->month = 0; days[datetime->month] < time; datetime->month++) {
        time -= days[datetime->month];
    }
    datetime->day = time;
}

static inline uint64_t timespec2ms(timespec_t *timespec) {
    uint64_t ret = timespec->tv_sec * 1000;
    ret += div_u64_rem(timespec->tv_nsec, 1000000, NULL);
    return ret;
}
#endif // TOOL_DATETIME_H
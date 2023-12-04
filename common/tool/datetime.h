#if !defined(TOOL_DATETIME_H)
#define TOOL_DATETIME_H

#include "../types/basic.h"

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

#endif // TOOL_DATETIME_H
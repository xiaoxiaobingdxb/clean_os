#if !defined(IO_STD_H)
#define IO_STD_H

#include "common/types/basic.h"

void init_std();

ssize_t printf(const char *fmt, ...);

ssize_t scanf(const char *fmt, ...);

#endif // IO_STD_H

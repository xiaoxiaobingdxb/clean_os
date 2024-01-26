#if !defined(IO_STD_H)
#define IO_STD_H

#include "common/types/basic.h"
#include "include/fs_model.h"

void init_std();

ssize_t fprintf(const fd_t fd, const char *fmt, ...);
ssize_t printf(const char *fmt, ...);

char getchar();

ssize_t gets(char *buf);

ssize_t scanf(const char *fmt, ...);

#endif // IO_STD_H

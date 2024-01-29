#if !defined(STDIO_H)
#define STDIO_H

#include "common/types/basic.h"
#include "include/fs_model.h"

extern void init_std();
extern void release_std();

extern ssize_t fprintf(const fd_t fd, const char *fmt, ...);
extern ssize_t printf(const char *fmt, ...);

extern char getchar();

extern ssize_t gets(char *buf);

extern ssize_t scanf(const char *fmt, ...);


#endif // STDIO_H

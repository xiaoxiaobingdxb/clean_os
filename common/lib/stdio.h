#ifndef LIB_STDIO_H
#define LIB_STDIO_H

#define va_start(ap, v) ap = (va_list)&v
#define va_arg(ap, t) *((t*)(ap += 4))
#define va_end(ap) ap = NULL
typedef char* va_list;

int vsprintf(char* str, const char* format, va_list ap);
int sprintf(char *str, const char *fmt, ...);
#endif
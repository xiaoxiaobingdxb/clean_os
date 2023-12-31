#ifndef LIB_STRING_H
#define LIB_STRING_H
#include "../types/basic.h"
const void *memset(const void *dst, uint8_t c, size_t len);
void memcpy(const void *dst, const void *src, size_t size);
const char *strcat(const char *dst, const char *src);
int memcmp(const void *a, const void *b, size_t size);
uint32_t strlen(const char *str);
char *strchr(const char *str, const char ch);
char *strrchr(const char *str, const char ch);
uint32_t strchrs(const char* str, const char ch);
#endif
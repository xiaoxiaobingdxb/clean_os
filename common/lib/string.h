#ifndef LIB_STRING_H
#define LIB_STRING_H
#include "../types/basic.h"
const void *memset(const void *dst, uint8_t c, size_t len);
void memcpy(const void *dst, const void *src, size_t size);
const char *strcat(const char *dst, const char *src);
int memcmp(const void *a, const void *b, size_t size);
size_t strcmp(const char *a, const char *b);
size_t strlen(const char *str);
size_t replace(char *str, char new, char old);
size_t replace_all(char *str, char new, char old);
size_t trim(char *str);
size_t trim_left(char *str);
size_t trim_right(char *str);
size_t trim_strlen(const char *str);
const char *strchr(const char *str, const char ch);
const char *strrchr(const char *str, const char ch);
uint32_t strchrs(const char *str, const char ch);
int str2num(const char *str, int *num);
char *strtok(char *str, const char *delim);
char *strtok_r(char *str, const char *delim, char **save_ptr);
#endif
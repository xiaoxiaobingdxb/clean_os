#ifndef TOOL_LIB_H
#define TOOL_LIB_H
#include "../cpu/contrl.h"
#define ASSERT(condition)    \
    if (!(condition)) panic(__FILE__, __LINE__, __func__, #condition)
void panic (const char * file, int line, const char * func, const char * cond);
#endif
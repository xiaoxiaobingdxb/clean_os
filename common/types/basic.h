#ifndef TYPES_BASIC_H
#define TYPES_BASIC_H

#include "std.h"

typedef unsigned char uint8_t;
typedef signed char sint8_t;
typedef uint8_t byte_t;
typedef unsigned short uint16_t;
typedef signed short sint16_6;
typedef unsigned int uint32_t;
typedef signed int sint32_t;
typedef unsigned long long uint64_t;
typedef signed long long sint64_t;

typedef __SIZE_TYPE__ size_t;
typedef long signed int ssize_t;
typedef long signed int off_t;

#define NULL ((void *)0)
#endif
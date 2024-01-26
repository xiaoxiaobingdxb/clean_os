#ifndef LIB_STACK_H
#define LIB_STACK_H

#include "common/types/basic.h"

void *stack_push(void *stack, void* value);
void *stack_push_value(void *stack,  void *value, size_t size);
void *stack_pop(void *stack, void* value);


#endif
#include "../types/basic.h"
#include "string.h"
void *stack_push(void *stack, void* value) {
    stack -= sizeof(void*);
    *((uint32_t*)stack) = (uint32_t)value;
    return stack;
}

void *stack_push_value(void *stack,  void *value, size_t size) {
    stack -= size;
    memcpy(stack, value, size);
    return stack;
}

void *stack_pop(void *stack, void* value) {
    *((uint32_t*)value) = *((uint32_t*)stack);
    return stack+sizeof(void*);
}
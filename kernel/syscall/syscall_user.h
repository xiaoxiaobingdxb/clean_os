#ifndef SYSCALL_SYSCALL_USER_H
#define SYSCALL_SYSCALL_USER_H
#include "common/types/basic.h"

void yield();
uint32_t get_pid();
int execve(const char *filename, char *const argv[], char *const envp[]);

#endif
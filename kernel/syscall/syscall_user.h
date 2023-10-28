#ifndef SYSCALL_SYSCALL_USER_H
#define SYSCALL_SYSCALL_USER_H
#include "common/types/basic.h"

void yield();
uint32_t get_pid();
uint32_t fork();
uint32_t wait(int *status);
void exit(int status);
int execve(const char *filename, char *const argv[], char *const envp[]);

#define PROT_NONE 0x0  /* page can not be accessed */
#define PROT_READ 0x1  /* page can be read */
#define PROT_WRITE 0x2 /* page can be written */
#define PROT_EXEC 0x4  /* page can be executed */

#define MAP_SHARED 0x01    /* Share changes */
#define MAP_PRIVATE 0x02   /* Changes are private */
#define MAP_ANONYMOUS 0x20 /* don't use a file */

void *mmap(void *addr, size_t length, int prot, int flags, int fd, int offset);

void clone(const char *name, uint32_t priority, void *func, void *func_arg);

#endif
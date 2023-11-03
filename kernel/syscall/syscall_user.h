#ifndef SYSCALL_SYSCALL_USER_H
#define SYSCALL_SYSCALL_USER_H
#include "common/types/basic.h"
#include "../thread/model.h"
#include "../thread/thread.h"

void yield();
pid_t get_pid();
pid_t get_ppid();
pid_t fork();
pid_t wait(int *status);
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

#define CLONE_VM (1 << 0) // set if vm shared between process
#define CLONE_PARENT (1 << 1) // set if want the same parent, also create a new process as the cloner' brother
#define CLONE_VFORK (1 << 2) // set if block current process until child process exit
uint32_t clone(int (*func)(void*), void* child_stack, int flags, void *func_arg);

void deprecated_clone(const char *name, uint32_t priority, void *func, void *func_arg);


int sysinfo(uint32_t pid, sys_info *info);

#endif
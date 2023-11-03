#ifndef THREAD_PROCESS_H
#define THREAD_PROCESS_H
#include "common/types/basic.h"
#include "../thread/model.h"
#include "thread.h"

typedef struct {
    uint32_t intr_no;
    // all register for popal
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp_dummy;	// not use, is declared to hold 4byte in stack for popal
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;

    // below field is declared to simulate interrupt iret, start process from kernel mode into user mode
    uint32_t err_code; // not use, is delcared to hold 4byte in stack for iret
    void (*eip) (void); // iret to the code address, really is process entry function
    uint32_t cs; // process cs selector
    uint32_t eflags;
    void* esp; // the really stack in user mode
    uint32_t ss; // process ss selector
} process_stack;

pid_t alloc_pid();

void process_execute(void *p_func, const char *name);

/**
 * @brief get current process pid
*/
pid_t process_get_pid();

/**
 * @brief get current process parent_pid
*/
pid_t process_get_ppid();

/**
 * @brief create a new child process from current process, new process will run at the current code
 * usage:
 * int pid = fork();
 * if (!pid) {
 *      run parent code
 * } else {
 *      run child code
 * }
 * @return if > 0 return to parent process, return child process pid 
 *         else if == 0 return to child process
 *         else if < 0 fail
*/
pid_t process_fork();

/**
 * @brief pause current process and wait for a child exit or hanging
 * @param [out] status child process exit status
 * @return child process pid
*/
pid_t process_wait(int *status);

/**
 * @brief process exit by itself
 * @param [in] status exit status, it will be received by parent process_wait
*/
void process_exit(int status);

/**
 * @brief clear all state in current and move current process to run the filename
 * @param [in] filename the program to be run
 * @param [in] argv program entry arg
 * @param [in] envp program other env
 * @return if fail 0 else success
*/
int process_execve(const char *filename, char *const argv[], char *const envp[]);

/**
 * map a virtual address to device
 * @param [in] addr virtual address, if fd is 0 and flags has MAP_ANONYMOUS, it will alloc length memory from virtual addr
 * if addr is 0, it will alloc available virtual address
 * @param [in] length map memory size, byte
 * @param [in] prot the memory privilege
 * @param [in] flags attribue
 * @param [in] offset addr is map to device offset from start
 * @return map return virtual address
*/
void *process_mmap(void *addr, size_t length, int prot, int flags, int fd, int offset);

int process_sysinfo(pid_t pid, sys_info *info);

pid_t process_clone(int (*func)(void *), void *child_stack, int flags,
               void *func_arg);

#endif
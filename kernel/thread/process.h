#ifndef THREAD_PROCESS_H
#define THREAD_PROCESS_H
#include "common/types/basic.h"

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

void process_execute(void *p_func, const char *name);

#endif
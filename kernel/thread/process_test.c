#include "common/types/basic.h"
#include "../syscall/syscall_user.h"
#include "common/cpu/mem_page.h"
#include "process.h"
int test_clone_process(void *arg) {
    const char *p = (const char*)arg;
    int count = 0;
    for (;;) {
        count++;
        if (count > 10) {
            break;
        }
        yield();
    }
    return 1;
}

void test_child_process(int *count) {
    uint32_t child_stack = (uint32_t)mmap(
        NULL, MEM_PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS, 0, 0);
    child_stack += MEM_PAGE_SIZE;
    uint32_t child_thread_id =
        clone(test_clone_process, (void *)child_stack, 0, "test");
    for (;;) {
        *count = *count + 2;
        uint32_t pid = get_pid();
        // int a = 10 / 0;
        yield();
        if (*count > 10) {
            break;
        }
    }
}

void test_fork_process() {
    int count = 0;
    sys_info info;
    pid_t pid = get_pid();
    sysinfo(pid, &info);
    pid_t child_pid = fork();
    if (!child_pid) {  // is child
        test_child_process(&count);
        exit(0);
        return;
    }
    for (;;) {
        // uint32_t child_stack = (uint32_t)mmap(NULL, MEM_PAGE_SIZE, PROT_READ
        // | PROT_WRITE,  MAP_ANONYMOUS, 0, 0); child_stack += MEM_PAGE_SIZE -1;
        // uint32_t child_thread_id = clone(test_clone_process,
        // (void*)child_stack, 0, NULL);

        int result = sysinfo(pid, &info);
        int status;
        child_pid = wait(&status);
        result = sysinfo(pid, &info);
        count++;
        if (child_pid == -1) {
            break;
        }
    }
}

void test_fork() {
    pid_t child_pid = fork();
    if (!child_pid) {
        test_fork_process();
        exit(0);
    }
}

typedef struct {
    const char *name;
    void *test_mem;
    pid_t clone_pid;
    pid_t clone_ppid;
}test_clone_args;
int test_clone_process1(void *arg) {
    test_clone_args *args = (test_clone_args*) arg;
    pid_t pid = get_pid();
    pid_t ppid = get_ppid();
    int count = 0;
    for (;;) {
        count++;
        if (count > 10) {
            break;
        }
        yield();
    }
    return 1;
}
test_clone_args args1, args2, args3, args4;
void test_clone_p() {
    pid_t pid = get_pid();
    pid_t ppid = get_ppid();
    args1.clone_pid = args2.clone_pid = args3.clone_pid = args4.clone_pid = pid;
    args1.clone_ppid = args2.clone_ppid = args3.clone_ppid = args4.clone_ppid = ppid;
    void *test_mem = mmap(NULL, MEM_PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS, 0, 0);
    *((uint32_t*)test_mem) = 1;
    args1.name = "test_clone_vm";
    args1.test_mem = test_mem;
    void *stack = mmap(NULL, MEM_PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS, 0, 0);
    pid_t clone_vm_pid = clone(test_clone_process1, stack, CLONE_VM, (void*)&args1);
    args2.name = "test_clone_vm2";
    args2.test_mem = test_mem;
    pid_t clone_vm_pid2 = clone(test_clone_process1, NULL, CLONE_VM, (void*)&args2);
    args3.name = "test_clone_parent";
    args3.test_mem = test_mem;
    pid_t clone_parent_pid = clone(test_clone_process1, NULL, CLONE_PARENT, (void*)&args3);
    args4.name ="test_clone_vfork";
    args4.test_mem = test_mem;
    pid_t clone_vfork_pid = clone(test_clone_process1, NULL, CLONE_VFORK, (void*)&args4);
    return;
}

void test_clone() {
    pid_t cpid = fork();
    if (!cpid) {
        test_clone_p();
        exit(0);
    }
}
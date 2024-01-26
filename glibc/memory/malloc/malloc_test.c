#include "malloc.h"

#include "include/syscall.h"
int test_malloc(void *arg) {
    pid_t pid = get_pid();
    sys_info info;
    sysinfo(pid, &info, SYS_INFO_MEM);
    int *arr = malloc(256 * sizeof(int));
    sysinfo(pid, &info, SYS_INFO_MEM);
    for (int i = 0; i < 10; i++) {
        arr[i] = i;
    }
    int *k_1 = malloc(256 * sizeof(int));
    int *k_2 = malloc(256 * sizeof(int));
    int *k_3 = malloc(256 * sizeof(int));
    int *k_4 = malloc(256 * sizeof(int));
    int *k_big = malloc(10000 * sizeof(int));
    sysinfo(pid, &info, SYS_INFO_MEM);
    free(k_1);
    sysinfo(pid, &info, SYS_INFO_MEM);
    free(k_2);
    sysinfo(pid, &info, SYS_INFO_MEM);
    free(k_3);
    sysinfo(pid, &info, SYS_INFO_MEM);
    free(k_4);
    sysinfo(pid, &info, SYS_INFO_MEM);
    free(k_big);
    sysinfo(pid, &info, SYS_INFO_MEM);
    free(arr);
    sysinfo(pid, &info, SYS_INFO_MEM);
    pid = get_pid();
    return 0;
}

void test_malloc_process() { clone(test_malloc, NULL, 0, NULL); }
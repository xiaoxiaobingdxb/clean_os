#include "malloc.h"

#include "../../syscall/syscall_user.h"
int test_malloc(void *arg) {
    pid_t pid = get_pid();
    sys_info info;
    sysinfo(pid, &info);
    int *arr = malloc(256 * sizeof(int));
    sysinfo(pid, &info);
    for (int i = 0; i < 10; i++) {
        arr[i] = i;
    }
    int *k_1 = malloc(256 * sizeof(int));
    int *k_2 = malloc(256 * sizeof(int));
    int *k_3 = malloc(256 * sizeof(int));
    int *k_4 = malloc(256 * sizeof(int));
    int *k_big = malloc(10000 * sizeof(int));
    sysinfo(pid, &info);
    free(k_1);
    sysinfo(pid, &info);
    free(k_2);
    sysinfo(pid, &info);
    free(k_3);
    sysinfo(pid, &info);
    free(k_4);
    sysinfo(pid, &info);
    free(k_big);
    sysinfo(pid, &info);
    free(arr);
    sysinfo(pid, &info);
    pid = get_pid();
    return 0;
}

void test_malloc_process() { clone(test_malloc, NULL, 0, NULL); }
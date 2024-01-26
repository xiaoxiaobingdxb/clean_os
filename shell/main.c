#include "glibc/include/stdio.h"
int main(int argc, char **argv) {
    printf("argc=%d,", argc);
    for (int i = 0; i < argc; i++) {
        printf("argv[%d]=%s,", i, argv[i]);
    }
    printf("\n");
    int a = 1;
    int b = 2;
    int c = a + b;
    printf("%d+%d=%d\n", a, b, c);
    return 0;
}
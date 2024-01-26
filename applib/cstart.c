#include "include/syscall.h"

int main(int argc, char **argv);
extern char bss_start[], bss_end[];

void cstart(int argc, char **argv) {
    
    char *start = bss_start;
    while (start < bss_end) {
        *start++ = 0;
    }
    exit(main(argc, argv));
}
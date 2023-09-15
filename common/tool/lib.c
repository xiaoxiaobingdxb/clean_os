#include "lib.h"
void panic(const char* file, int line, const char* func, const char* cond) {
    for (;;) {
        hlt();
    }
}
#include "thread.h"
#include "../interrupt/intr.h"
#include "../interrupt/intr_no.h"

void timer_handler(uint32_t intr_no) {
    schedule();
}
void init_timer() {
    register_intr_handler(INTR_NO_TIMER, timer_handler);
}
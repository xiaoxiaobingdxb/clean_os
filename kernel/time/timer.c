#include "../thread/thread.h"
#include "../interrupt/intr.h"
#include "../interrupt/intr_no.h"
#include "pit.h"

void timer_pit_handler(uint32_t intr_no) {
    pit_tick();
    schedule();
}
void init_schedule_timer() {
    register_intr_handler(INTR_NO_TIMER, timer_pit_handler);
}
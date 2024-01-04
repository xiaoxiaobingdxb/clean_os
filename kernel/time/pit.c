#include "common/cpu/contrl.h"
#include "time.h"

#define PIT_OSC_FREQ 1193182

#define PIT_CHANNEL0_DATA_PORT 0x40
#define PIT_COMMAND_MODE_PORT 0x43

#define PIT_CHANNLE0 (0 << 6)
#define PIT_LOAD_LOHI (3 << 4)
#define PIT_MODE0 (3 << 1)
void pit_init() {
    int reload_count = PIT_OSC_FREQ / HZ;

    outb(PIT_COMMAND_MODE_PORT, PIT_CHANNLE0 | PIT_LOAD_LOHI | PIT_MODE0);
    outb(PIT_CHANNEL0_DATA_PORT, reload_count & 0xFF);
    outb(PIT_CHANNEL0_DATA_PORT, (reload_count >> 8) & 0xFF);
}

uint64_t jiffies = 0;
void pit_tick() {
    jiffies++;
}
uint64_t get_jiffies() {
    return jiffies;
}
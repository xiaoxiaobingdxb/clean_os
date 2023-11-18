#if !defined(DEVICE_KEYBOARD_H)
#define DEVICE_KEYBOARD_H

#include "common/lib/block_queue.h"

void init_keyboard();
extern block_queue kbd_buf;

#endif // DEVICE_KEYBOARD_H

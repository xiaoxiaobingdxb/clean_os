#include "../../interrupt/intr.h"
#include "../../interrupt/intr_no.h"
#include "common/cpu/contrl.h"
#include "common/lib/block_queue.h"
#include "common/tool/log.h"
#include "common/types/std.h"
#include "keyboard_code.h"
#include "../../thread/schedule/segment.h"

#define KBD_DATA_PORT 0x60
#define KBD_STATUS_PORT 0x64

bool ext_scancode = false;
bool ctrl_down, shift_down, alt_down, caps_lock_down;

block_queue kbd_buf;
char buf[queue_buf_size];
segment_t lock;

void keyboard_hander(uint32_t intr_no) {
    io_wait();
    uint16_t scan_code = inb(KBD_DATA_PORT);
    if (scan_code == 0xe0) {
        ext_scancode = true;
        return;
    }
    if (ext_scancode) {
        scan_code = 0xe000 | scan_code;
        ext_scancode = false;
    }
    bool break_code = (scan_code & 0x0080) != 0;
    if (break_code) {
        uint16_t make_code = (scan_code &= 0xff7f);
        switch (make_code) {
        case ctrl_l_make:
        case ctrl_r_make:
            ctrl_down = false;
            break;
        case shift_l_make:
        case shift_r_make:
            shift_down = false;
            break;
        case alt_l_make:
        case alt_r_make:
            alt_down = false;
            break;
        default:
            break;
        }
        return;
    }
    if (scan_code > 0x0 && scan_code < 0x3b || scan_code == alt_r_make ||
        scan_code == ctrl_r_make) {
        bool shift = false;
        if (shift_down) {
            shift = true;
        }
        uint8_t idx = (scan_code &= 0x00ff);
        char ch = keymap[idx][shift];
        if (ch) {
            _log_debug("keyboard input ch=%c\n", ch) int idx = queue_len(&kbd_buf);
            buf[idx] = ch;
            queue_put(&kbd_buf, (void*)buf + idx);
            return;
        }
        switch (scan_code) {
        case ctrl_l_make:
        case ctrl_r_make:
            ctrl_down = true;
            break;
        case shift_l_make:
        case shift_r_make:
            shift_down = true;
            break;
        case alt_l_make:
        case alt_r_make:
            alt_down = true;
            break;
        case caps_lock_make:
            caps_lock_down = !caps_lock_down;
            break;
        default:
            break;
        }
        return;
    }
}

void wait_fun(int *i) {
    segment_wait(&lock, (pid_t*)i);
}
void wakeup_fun(int *i) {
    segment_wakeup(&lock, (pid_t*)i);
}
bool inited = false;
void init_keyboard() {
    if (!inited) {
        register_intr_handler(INTR_NO_KYB, keyboard_hander);
        init_segment(&lock, 0);
        init_queue(&kbd_buf, wait_fun, wakeup_fun);
        inited = true;
    }
}

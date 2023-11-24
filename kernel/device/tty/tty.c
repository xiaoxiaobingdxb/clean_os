#include "./tty.h"

#include "./console.h"
#include "../device.h"
#include "keyboard.h"
#include "keyboard_code.h"

#define TTY_IN_N_RN 1 << 0  // 输入时\n转换成\n\r实现换行+回车
#define TTY_OUT_R_N 1 << 10  // 输出时\r转换成\n，实现回车转换成换行
#define TTY_OUT_ECHO 1 << 11
#define TTY_OUT_LINE 1 << 12

typedef struct {
    int console_idx;
    int in_flags, out_flags;
} tty_t;

#define TTY_SIZE 8
tty_t ttys[TTY_SIZE];

error tty_open(device_t *dev) {
    if (dev->minor < 0 || dev->minor >= TTY_SIZE) {
        return -1;
    }
    tty_t *tty = ttys + dev->minor;
    tty->console_idx = dev->minor;
    int flags = 0;
    if (dev->arg != NULL) {
        flags = *((int *)dev->arg);
    } else {
        flags = TTY_IN_N_RN | TTY_OUT_ECHO | TTY_OUT_R_N;
    }
    tty->in_flags = flags;
    tty->out_flags = flags;
    init_console(dev->minor);
    init_keyboard();
    return 0;
}
tty_t *get_tty(device_t *dev) {
    if (dev->minor < 0 || dev->minor >= TTY_SIZE || dev->open_count <= 0) {
        return NULL;
    }
    return ttys + dev->minor;
}
ssize_t tty_write(device_t *dev, uint32_t addr, const byte_t *buf,
                  size_t size) {
    tty_t *tty = get_tty(dev);
    if (!tty) {
        return -1;
    }
    for (int i = 0; i < size; i++) {
        switch (buf[i]) {
        case '\n':
            if (tty->in_flags | TTY_IN_N_RN) {
                console_putchar(tty->console_idx, '\r');
            }
            break;
        default:
            break;
        }
        console_putchar(tty->console_idx, (char)buf[i]);
        console_update_cursor_pos(tty->console_idx);
    }
    return size;
}
ssize_t tty_read(device_t *dev, uint32_t addr, byte_t *buf, size_t size) {
    tty_t *tty = get_tty(dev);
    if (tty == NULL) {
        return -1;
    }
    size_t len = 0;
    while (len < size) {
        char *value = (char *)queue_get(&kbd_buf);
        if (value == NULL) {
            continue;
        }
        if (*value == '\r' && (tty->out_flags & TTY_OUT_R_N)) {
            *value = '\n';
        }
        if (tty->out_flags & TTY_OUT_ECHO) {
            tty_write(dev, 0, value, 1);
        }
        buf[len++] = *((byte_t *)value);
        if ((*value) == '\n' && (tty->out_flags & TTY_OUT_LINE)) {
            break;
        }
    }
    return len;
}
int tty_control(device_t *dev, int cmd, int arg0, int arg1) { return 0; }
void tty_close(device_t *dev) {}

device_desc_t dev_tty_desc = {.name = "tty",
                              .major = DEV_TTY,
                              .open = tty_open,
                              .write = tty_write,
                              .read = tty_read,
                              .control = tty_control,
                              .close = tty_close};
#include "./tty.h"

#include "./console.h"
#include "./device.h"
#include "keyboard.h"

typedef struct {
    int console_idx;
} tty_t;

#define TTY_SIZE 8
tty_t ttys[TTY_SIZE];

int tty_open(device_t *dev) {
    if (dev->minor < 0 || dev->minor >= TTY_SIZE) {
        return -1;
    }
    ttys[dev->minor].console_idx = dev->minor;
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
ssize_t tty_write(device_t *dev, uint32_t addr, const byte_t *buf, size_t size) {
    tty_t *tty = get_tty(dev);
    if (!tty) {
        return -1;
    }
    for (int i = 0; i < size; i++) {
        switch (buf[i])
        {
        case '\n':
            console_putstr(tty->console_idx, "\r\n", 2);
            break;
        
        default:
        console_putchar(tty->console_idx, (char)buf[i]);
            break;
        }
    }
    return size;
}
ssize_t tty_read(device_t *dev, uint32_t addr, const byte_t *buf, size_t size) {
    return 0;
}
int tty_control(device_t *dev, int cmd, int arg0, int arg1) {
    return 0;
}
void tty_close(device_t *dev) {

}

device_desc_t dev_tty_desc = {.name = "tty",
                              .major = DEV_TTY,
                              .open = tty_open,
                              .write = tty_write,
                              .read = tty_read,
                              .control = tty_control,
                              .close = tty_close};
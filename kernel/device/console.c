#include "./console.h"

#include "common/cpu/contrl.h"
#include "common/lib/string.h"
#include "common/types/basic.h"
#include "common/types/std.h"

#define CONSOLE_DISPLAY_ADDR 0xb8000
#define CONSOLE_CNT 8
#define CONSOLE_COL_MAX 80
#define CONSOLE_ROW_MAX 25

console_t console_buf[CONSOLE_CNT];

/**
 * @see https://wiki.osdev.org/Text_Mode_Cursor
 */
int read_cursor_pos() {
    uint16_t pos = 0;
    outb(0x3D4, 0x0F);
    pos |= inb(0x3D5);
    outb(0x3D4, 0x0E);
    pos |= ((uint16_t)inb(0x3D5)) << 8;
    return pos;
}

void update_cursor_pos(console_t *console) {
    uint16_t pos = ((uint32_t)console - (uint32_t)console_buf) /
                   sizeof(console_t) *
                   (console->display_cols * console->display_rows);
    pos += console->cursor_row * console->display_cols + console->cursor_col;

    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void clear(console_t *console, int row_start, int row_end) {
    display_char_t *display_start =
        console->display_base + console->display_cols * row_start;
    display_char_t *display_end =
        console->display_base + console->display_cols * (row_end + 1);
    for (; display_start < display_end; display_start++) {
        display_start->v = 0;
    }
}

void init_console(int idx) {
    console_t *console = console_buf + idx;
    console->display_cols = CONSOLE_COL_MAX;
    console->display_rows = CONSOLE_ROW_MAX;
    console->display_fg = Gray;
    console->display_bg = Black;
    console->display_base = (display_char_t *)(CONSOLE_DISPLAY_ADDR +
                                               idx * (console->display_rows *
                                                      console->display_cols));
    if (idx == 0) {
        int cur_pos = read_cursor_pos();
        console->cursor_row = cur_pos / console->display_cols;
        console->cursor_col = cur_pos % console->display_cols;
    } else {
        console->cursor_row = 0;
        console->cursor_col = 0;
        clear(console, 0, console->display_rows);
    }
}

void scroll_up(console_t *console, int lines) {
    uint32_t size = (console->display_rows - lines) * console->display_cols *
                    sizeof(display_char_t);
    memcpy(console->display_base,
           console->display_base + console->display_cols * lines, size);
    clear(console, console->display_rows - lines, console->display_rows - 1);
    console->cursor_row -= lines;
}

void cursor_forward(console_t *console, int n) {
    for (int i = 0; i < n; i++) {
        if (++console->cursor_col >= console->display_cols) {
            console->cursor_col = 0;
            console->cursor_row++;
            if (console->cursor_row >= console->display_rows) {
                scroll_up(console, 1);
            }
        }
    }
}

void cursor_backward(console_t *console, int n) {
    for (int i = 0; i < n; i++) {
        if (console->cursor_col > 0) {
            console->cursor_col--;
        } else if (console->cursor_row > 0) {
            console->cursor_row--;
            console->cursor_col = console->display_cols - 1;
        }
    }
}

void cursor_up(console_t *console, int n) {
    for (int i = 0; i < n; i++) {
        if (console->cursor_row > 0) {
            console->cursor_row--;
        }
    }
}

void cursor_down(console_t *console, int n) {
    for (int i = 0; i < n; i++) {
        if (console->cursor_row > 0) {
            console->cursor_row++;
        }
        if (console->cursor_row >= console->display_rows) {
            scroll_up(console, 1);
        }
    }
}

void cursor_line_head(console_t *console) { console->cursor_col = 0; }

void putchar_color(console_t *console, char ch, console_color fg,
                   console_color bg) {
    int offset =
        console->cursor_col + console->cursor_row * console->display_cols;
    display_char_t *display = console->display_base + offset;
    display->ch = ch;
    display->fg = fg;
    display->bg = bg;
    cursor_forward(console, 1);
}

void putchar(console_t *console, char ch) {
    putchar_color(console, ch, console->display_fg, console->display_bg);
}

void console_putchar(int idx, char ch) {
    if (idx < 0 || idx >= CONSOLE_CNT) {
        return;
    }
    console_t *console = console_buf + idx;
    switch (ch) {
    case '\n':
        cursor_down(console, 1);
        break;
    case '\r':
        cursor_line_head(console);
        break;
    case '\b':
        cursor_backward(console, 1);
        putchar(console, ' ');
        cursor_backward(console, 1);
        break;
    case '\177':
        putchar(console, ' ');
        cursor_backward(console, 1);
        break;

    default:
        if (ch >= ' ' && ch <= '~') {
            putchar(console, ch);
        }
        break;
    }
}

void console_update_cursor_pos(int idx) {
    if (idx < 0 || idx >= CONSOLE_CNT) {
        return;
    }
    console_t *console = console_buf + idx;
    update_cursor_pos(console);
}

void console_putstr(int idx, char *str, size_t size) {
    for (int i = 0; i < size; i++) {
        console_putchar(idx, str[i]);
    }
}

void console_cursor_forward(int idx, int n) {
    if (idx < 0 || idx >= CONSOLE_CNT) {
        return;
    }
    console_t *console = console_buf + idx;
    cursor_forward(console, n);
}

void console_cursor_backward(int idx, int n) {
    if (idx < 0 || idx >= CONSOLE_CNT) {
        return;
    }
    console_t *console = console_buf + idx;
    cursor_backward(console, n);
}
#include "./console.h"

#include "./keyboard_code.h"
#include "common/cpu/contrl.h"
#include "common/lib/string.h"
#include "common/types/basic.h"
#include "common/types/std.h"

#define CONSOLE_DISPLAY_ADDR 0xb8000
#define CONSOLE_CNT 8
#define CONSOLE_COL_MAX 80
#define CONSOLE_ROW_MAX 25
#define DEFAULT_FG Gray
#define DEFAULT_BG Black

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

void clear(console_t *console, int row_start, int row_end, int col_start,
           int col_end) {
    display_char_t *display_start =
        console->display_base + console->display_cols * row_start + col_start;
    display_char_t *display_end =
        console->display_base + console->display_cols * (row_end - 1) + col_end;
    for (; display_start < display_end; display_start++) {
        display_start->ch = ' ';
    }
}

void clear_row(console_t *console, int row_start, int row_end) {
    clear(console, row_start, row_end, 0, console->display_cols);
}

void init_console(int idx) {
    console_t *console = console_buf + idx;
    console->display_cols = CONSOLE_COL_MAX;
    console->display_rows = CONSOLE_ROW_MAX;
    console->display_fg = DEFAULT_FG;
    console->display_bg = DEFAULT_BG;
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
        clear_row(console, 0, console->display_rows);
    }
}

void scroll_up(console_t *console, int lines) {
    uint32_t size = (console->display_rows - lines) * console->display_cols *
                    sizeof(display_char_t);
    memcpy(console->display_base,
           console->display_base + console->display_cols * lines, size);
    clear_row(console, console->display_rows - lines,
              console->display_rows - 1);
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
        if (console->cursor_row <= console->display_rows ) {
            console->cursor_row++;
        }
        if (console->cursor_row >= console->display_rows) {
            scroll_up(console, 1);
        }
    }
}

void cursor_line_head(console_t *console) { console->cursor_col = 0; }

void set_color(console_t *console, console_color fg, console_color bg) {
    console->display_fg = fg;
    console->display_bg = bg;
}

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

void putchar_normal(console_t *console, char ch) {
    switch (ch) {
    case '\n':
        cursor_down(console, 1);
        break;
    case enter:
        cursor_line_head(console);
        break;
    case backspace:
        cursor_backward(console, 1);
        putchar(console, ' ');
        cursor_backward(console, 1);
        break;
    case delete:
        putchar(console, ' ');
        cursor_backward(console, 1);
        break;
    case tab:
        cursor_forward(console, 4);
        break;
    case esc:
        console->write_state = WRITE_STATE_ESC;
        break;
    default:
        if (ch >= ' ' && ch <= '~') {
            putchar(console, ch);
        }
        break;
    }
}

void putchar_esc(console_t *console, char ch) {
    switch (ch) {
    case '[':
        memset(console->esc_params, 0, sizeof(console->esc_params));
        console->cur_esc_param = 0;
        console->write_state = WRITE_STATE_SQUARE;
        break;
    default:
        console->write_state = WRITE_STATE_NORMAL;
        break;
    }
}

const console_color color_table[] = {Black, Red,    Green, Yellow,
                                     Blue,  Purple, Cyan,  White};
void esc_set_color(console_t *console, int param) {
    if (param >= 30 && param <= 37) {
        set_color(console, color_table[param - 30], console->display_bg);
    } else if (param == 39) {
        set_color(console, DEFAULT_FG, console->display_bg);
    } else if (param >= 40 && param <= 47) {
        set_color(console, console->display_fg, color_table[param - 40]);
    } else if (param == 49) {
        set_color(console, console->display_fg, DEFAULT_BG);
    }
}
/**
 * @see https://en.wikipedia.org/wiki/ANSI_escape_code
 */
void putchar_esc_square(console_t *console, char ch) {
    if (ch >= '0' && ch <= '9') {
        int *num = console->esc_params + console->cur_esc_param;
        *num = *num * 10 + ch - '0';
    } else if (ch == ';' && console->cur_esc_param < ESC_PARAMS_MAX) {
        console->cur_esc_param++;
    } else {
        console->cur_esc_param++;
        switch (ch) {
        case 'm':
            for (int i = 0; i < console->cur_esc_param; i++) {
                esc_set_color(console, console->esc_params[i]);
            }
            break;
        case 'J':
            switch (console->esc_params[console->cur_esc_param - 1]) {
            case 0:
                clear(console, console->cursor_row, console->display_rows,
                      console->cursor_col, console->display_cols);
                break;
            case 1:
                clear(console, 0, console->cursor_row, 0, console->cursor_col);
                break;
            case 2:
                clear_row(console, 0, console->display_rows);
                console->cursor_row = 0;
                console->cursor_col = 0;
                break;
            case 3:
                clear_row(console, 0, console->display_rows);
                break;
            default:
                break;
            }
            break;
        default:
            break;
        }
        console->write_state = WRITE_STATE_NORMAL;
    }
}

size_t console_putchar(int idx, char ch) {
    if (idx < 0 || idx >= CONSOLE_CNT) {
        return 0;
    }
    console_t *console = console_buf + idx;
    switch (console->write_state) {
    case WRITE_STATE_NORMAL:
        putchar_normal(console, ch);
        return 1;
    case WRITE_STATE_ESC:
        putchar_esc(console, ch);
        return 0;
    case WRITE_STATE_SQUARE:
        putchar_esc_square(console, ch);
        return 0;
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
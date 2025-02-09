#ifndef DEVICE_CONSOLE_H
#define DEVICE_CONSOLE_H

#include "common/types/basic.h"

#define ESC_PARAMS_MAX 10

/**
 * @see https://wiki.osdev.org/Printing_To_Screen
 */

typedef enum {
    Black = 0,
    Blue,
    Green,
    Cyan,
    Red,
    Purple,
    Brown,
    Gray,
    Dark_Gray,
    Light_Blue,
    Light_Green,
    Light_Cyan,
    Light_Red,
    Light_Purple,
    Yellow,
    White
} console_color;
void init_console(int idx);

typedef union {
    struct {
        char ch;
        uint8_t fg : 4;
        uint8_t bg : 3;
    };
    uint16_t v;

} display_char_t;

typedef struct {
    int display_cols, display_rows;  // console width and height
    int cursor_col, cursor_row;      // cursor currently position
    console_color display_fg, display_bg;  // console default color
    display_char_t *display_base;
    enum {
        WRITE_STATE_NORMAL,
        WRITE_STATE_ESC,
        WRITE_STATE_SQUARE,
    } write_state;
    int esc_params[ESC_PARAMS_MAX];
    size_t cur_esc_param;
} console_t;
void console_update_cursor_pos(int idx);
size_t console_putchar(int idx, char ch);
void console_putstr(int idx, char *str, size_t size);
void console_cursor_backward(int idx, int n);
void console_cursor_forward(int idx, int n);
#endif
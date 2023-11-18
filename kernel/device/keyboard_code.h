#if !defined(DEVICE_KEYBOARD_CODE_H)
#define DEVICE_KEYBOARD_CODE_H

#define esc '\033'
#define backspace '\b'
#define tab '\t'
#define enter '\r'
#define delete '\177'

#define ctrl_l 0
#define ctrl_r 0
#define shift_l 0
#define shift_r 0
#define alt_l 0
#define alt_r 0
#define caps_lock 0

static char keymap[][2] = {{0, 0},                   // 0x0
                    {esc, esc},               // 0x1
                    {'1', '!'},               // 0x2
                    {'2', '@'},               // 0x3
                    {'3', '#'},               // 0x4
                    {'4', '$'},               // 0x5
                    {'5', '%'},               // 0x6
                    {'6', '^'},               // 0x7
                    {'7', '&'},               // 0x8
                    {'8', '*'},               // 0x9
                    {'9', '('},               // 0xa
                    {'0', ')'},               // 0xb
                    {'-', '_'},               // 0xc
                    {'=', '+'},               // 0xd
                    {backspace, backspace},   // 0xe
                    {tab, tab},               // 0xf
                    {'q', 'Q'},               // 0x10
                    {'w', 'W'},               // 0x11
                    {'e', 'E'},               // 0x12
                    {'r', 'R'},               // 0x13
                    {'t', 'T'},               // 0x14
                    {'y', 'Y'},               // 0x15
                    {'u', 'U'},               // 0x16
                    {'i', 'I'},               // 0x17
                    {'o', 'O'},               // 0x18
                    {'p', 'P'},               // 0x19
                    {'[', '{'},               // 0x1a
                    {']', '}'},               // 0x1b
                    {enter, enter},           // 0x1c
                    {ctrl_l, ctrl_l},         // 0x1d
                    {'a', 'A'},               // 0x1e
                    {'s', 'S'},               // 0x1f
                    {'d', 'D'},               // 0x20
                    {'f', 'F'},               // 0x21
                    {'g', 'G'},               // 0x22
                    {'h', 'H'},               // 0x23
                    {'j', 'J'},               // 0x24
                    {'k', 'K'},               // 0x25
                    {'l', 'L'},               // 0x26
                    {';', ':'},               // 0x27
                    {'\'', '"'},              // 0x028
                    {'`', '~'},               // x029
                    {shift_l, shift_l},       // 0x2a
                    {'\\', '|'},              // 0x2b
                    {'z', 'Z'},               // 0x2c
                    {'x', 'X'},               // 0x2d
                    {'c', 'C'},               // 0x2e
                    {'v', 'V'},               // 0x2f
                    {'b', 'B'},               // 0x30
                    {'n', 'N'},               // 0x31
                    {'m', 'M'},               // 0x32
                    {',', '<'},               // 0x33
                    {'.', '>'},               // 0x34
                    {'/', '?'},               // 0x35
                    {shift_r, shift_r},       // 0x36
                    {'*', '*'},               // 0x37
                    {alt_l, alt_l},           // 0x38
                    {' ', ' '},               // 0x39
                    {caps_lock, caps_lock}};  // 0x3a

#define ctrl_l_make 0x1d
#define ctrl_r_make 0xe01d
#define shift_l_make 0x2a
#define shift_r_make 0x36
#define alt_l_make 0x38
#define alt_r_make 0xe038
#define caps_lock_make 0x3a

#endif  // DEVICE_KEYBOARD_CODE_H

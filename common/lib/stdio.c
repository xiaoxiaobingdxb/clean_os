#include "./stdio.h"

#include "../tool/math.h"
#include "../types/basic.h"
#include "./string.h"

void itoa(uint64_t value, char** buf_ptr_addr, uint32_t base) {
    uint32_t m;
    uint64_t i = div_u64_rem(value, base, &m);
    // uint32_t m = value % base;
    // uint32_t i = value / base;
    if (i) {
        itoa(i, buf_ptr_addr, base);
    }
    if (m < 10) {
        *((*buf_ptr_addr)++) = m + '0';       // '0'~'9'
    } else {                                  // 否则余数是A~F
        *((*buf_ptr_addr)++) = m - 10 + 'A';  //'A'~'F'
    }
}

int vsprintf(char* str, const char* format, va_list ap) {
    char* buf_ptr = str;
    const char* index_ptr = format;
    char index_char = *index_ptr;
    int arg_int;
    char* arg_str;
    while (index_char) {
        if (index_char != '%') {
            *(buf_ptr++) = index_char;
            index_char = *(++index_ptr);
            continue;
        }
        index_char = *(++index_ptr);
        switch (index_char) {
        case 's':
            arg_str = va_arg(ap, char*);
            memcpy(buf_ptr, arg_str, strlen(arg_str));
            buf_ptr += strlen(arg_str);
            index_char = *(++index_ptr);
            break;

        case 'c':
            *(buf_ptr++) = va_arg(ap, char);
            index_char = *(++index_ptr);
            break;

        case 'd':
            arg_int = va_arg(ap, int);
            if (arg_int < 0) {
                arg_int = 0 - arg_int;
                *buf_ptr++ = '-';
            }
            itoa(arg_int, &buf_ptr, 10);
            index_char = *(++index_ptr);
            break;
        case 'x':
            arg_int = va_arg(ap, int);
            itoa(arg_int, &buf_ptr, 16);
            index_char = *(++index_ptr);
            break;
        case 'l':
            signed long long arg_long = va_arg(ap, signed long long);
            ap += 4;
            if (arg_long < 0) {
                arg_long = 0 - arg_long;
                *buf_ptr++ = '-';
            }
            // uint32_t high = arg_long >> 32;
            // uint32_t low = arg_long & 0xffffffff;
            // if (high > 0) {
            //     itoa(high, &buf_ptr, 10);
            // }
            // const size_t int32_len = 10;
            // char tmp[int32_len];
            // memset(tmp, 0, int32_len);
            // char* p = tmp;
            // itoa(low, &p, 10);
            // size_t len = strlen(tmp);
            // len = min(len, int32_len);
            // memset(buf_ptr, '0', int32_len - len);
            // buf_ptr += int32_len - len;
            // memcpy(buf_ptr, tmp, len);
            // buf_ptr += len;
            itoa(arg_long, &buf_ptr, 10);
            index_char = *(++index_ptr);
            break;
        }
    }
    return strlen(str);
}
int sprintf(char* buf, const char* format, ...) {
    va_list args;
    int retval;
    va_start(args, format);
    retval = vsprintf(buf, format, args);
    va_end(args);
    return retval;
}
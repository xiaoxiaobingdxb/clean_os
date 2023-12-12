#include "string.h"

#include "../tool/math.h"
const void *memset(const void *dst, uint8_t c, size_t len) {
    const unsigned char uc = c;
    unsigned char *su;
    for (su = (unsigned char *)dst; 0 < len; ++su, --len) {
        *su = uc;
    }
    return dst;
}

void memcpy(const void *dst, const void *src, size_t size) {
    uint8_t *dst_ = (uint8_t *)dst;
    const uint8_t *src_ = src;
    while (size-- > 0) {
        *dst_++ = *src_++;
    }
}

const char *strcat(const char *dst, const char *src) {
    char *str = (char *)dst;
    char *src_ = (char *)src;
    while (*str++)
        ;
    --str;
    while ((*str++ = *src_++))
        ;
    return dst;
}

int memcmp(const void *a, const void *b, size_t size) {
    const uint8_t *a_ = a;
    const uint8_t *b_ = b;
    while (size-- > 0) {
        if (*a_ != *b_) {
            return *a_ > *b_ ? 1 : -1;
        }
        a_++;
        b_++;
    }
    return 0;
}

size_t strlen(const char *str) {
    const char *p = str;
    while (*p++)
        ;
    return (p - str - 1);
}

size_t replace(const char *str, char new, char old) {
    size_t count = 0;
    for (int i = 0; i < strlen(str); i++) {
        if (str[i] == old) {
            *((char *)str + i) = new;
            count++;
        }
    }
    return count;
}

size_t trim(const char *str) { return replace(str, 0, ' '); }

size_t trim_strlen(const char *str) {
    const char *p = str;
    while (p > 0 && *p != ' ') {
        p++;
    }
    return p - str;
}

const char *strchr(const char *str, const char ch) {
    while (*str != 0) {
        if (*str == ch) {
            return (char *)str;
        }
        str++;
    }
    return NULL;
}
const char *strrchr(const char *str, const char ch) {
    const char *last = NULL;
    while (*str != 0) {
        if (*str == ch) {
            last = str;
        }
        str++;
    }
    return (const char *)last;
}

uint32_t strchrs(const char *str, const char ch) {
    uint32_t ch_cnt = 0;
    const char *p = str;
    while (*p != 0) {
        if (*p == ch) {
            ch_cnt++;
        }
    }
    return ch_cnt;
}

void strreverse(char *str, const size_t size) {
    char tmp;
    for (int i = 0; i < size / 2; i++) {
        tmp = str[i];
        str[i] = str[size - 1 - i];
        str[size - 1 - i] = tmp;
    }
}

int str2num(const char *str, int *num) {
    size_t size = strlen(str);
    if (size < 1) {
        return -1;
    }
    int signe = 1;
    if (str[0] == '-') {
        signe = -1;
        str++;
        size--;
    }
    int ret = 0;
    for (int i = size - 1, j = 0; i >= 0; i--, j++) {
        char ch = str[i];
        if (ch < '0' || ch > '9') {
            return -1;
        }
        int n = str[i] - '0';
        ret += n * pow(10, j);
    }
    if (num != NULL) {
        *num = signe * ret;
    }
    return 0;
}

void num2str(const int num, char *str) {
    int number = num;
    if (number < 0) {
        *(str++) = '-';
        number *= -1;
    }
    char buf[64];
    int i;
    for (i = 0; number > 0; i++, number /= 10) {
        buf[i] = number % 10 + '0';
    }
    strreverse(buf, i);
    memcpy(str, buf, i);
}

char *save_ptr;
char *strtok(char *str, const char *delim) {
    return strtok_r(str, delim, &save_ptr);
}

size_t strspn(const char *s, const char *accept) {
    const char *p;

    for (p = s; *p != '\0'; ++p) {
        if (!strchr(accept, *p))
            break;
    }
    return p - s;
}

size_t strcspn(const char *s, const char *reject) {
    const char *p;

    for (p = s; *p != '\0'; ++p) {
        if (strchr(reject, *p))
            break;
    }
    return p - s;
}

char *strtok_r(char *str, const char *delim, char **save_ptr) {
    char *end;

    if (str == NULL)
        str = *save_ptr;

    if (*str == '\0') {
        *save_ptr = str;
        return NULL;
    }

    /* Scan leading delimiters.  */
    str += strspn(str, delim);
    if (*str == '\0') {
        *save_ptr = str;
        return NULL;
    }

    /* Find the end of the token.  */
    end = str + strcspn(str, delim);
    if (*end == '\0') {
        *save_ptr = end;
        return str;
    }

    /* Terminate the token and make *SAVE_PTR point past it.  */
    *end = '\0';
    *save_ptr = end + 1;
    return str;
}
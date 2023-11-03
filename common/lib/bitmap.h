#ifndef LIB_BITMAP_H
#define LIB_BITMAP_H
#include "../types/basic.h"
#include "../types/std.h"

typedef struct {
    uint32_t bytes_len;
    uint8_t *bits;
} bitmap_t;

void bitmap_init(bitmap_t *btm);
bool bitmap_scan_test(bitmap_t *btm, uint32_t bit_idx);
int bitmap_scan(bitmap_t *btm, uint32_t cnt);
void bitmap_set(bitmap_t *btm, uint32_t bit_idx, uint8_t value);
typedef bool(bitmap_foreach_visitor)(bitmap_t *btm, uint32_t bit_idx, void *arg);
void bitmap_foreach(bitmap_t *btm, uint8_t fast_value, bitmap_foreach_visitor visitor, void *arg);

#endif
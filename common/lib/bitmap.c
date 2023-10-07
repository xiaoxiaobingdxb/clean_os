#include "bitmap.h"

#include "string.h"
#define BITMAP_MASK 1
void bitmap_init(bitmap_t *btm) { memset(btm->bits, 0, btm->bytes_len); }
bool bitmap_scan_test(bitmap_t *btm, uint32_t bit_idx) {
    uint32_t byte_idx = bit_idx / 8;
    uint32_t bit_odd = bit_idx % 8;
    return (btm->bits[byte_idx] & (BITMAP_MASK << bit_odd));
}

int bitmap_scan(bitmap_t *btm, uint32_t cnt) {
    uint32_t byte_idx = 0;
    while (btm->bits[byte_idx] == 0xff && byte_idx < btm->bytes_len) {
        byte_idx++;
    }
    if (byte_idx >= btm->bytes_len) {
        return -1;
    }
    int bit_idx = 0;
    while (btm->bits[byte_idx] & (BITMAP_MASK << bit_idx)) {
        bit_idx++;
    }
    if (cnt == 1) {
        return byte_idx * 8 + bit_idx;
    }
    uint32_t bit_left = btm->bytes_len * 8 - (byte_idx * 8 + bit_idx);
    uint32_t next_bit = byte_idx * 8 + bit_idx + 1;
    uint32_t find_cnt = 1;
    while (bit_left-- > 0) {
        if (!bitmap_scan_test(btm, next_bit)) {
            find_cnt++;
        } else {
            find_cnt = 0;
        }
        if (find_cnt == cnt) {
            return next_bit - cnt + 1;
        }
        next_bit++;
    }
    return -1;
}

void bitmap_set(bitmap_t *btm, uint32_t bit_idx, uint8_t value) {
    uint32_t byte_idx = bit_idx / 8;
    uint32_t bit_odd = bit_idx % 8;
    if (value) {
        btm->bits[byte_idx] |= BITMAP_MASK << bit_odd;
    } else {
        btm->bits[byte_idx] &= ~(BITMAP_MASK << bit_odd);
    }
}
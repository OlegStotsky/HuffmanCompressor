#include "utils.h"

uint8_t char_to_uint8_t(char sym) {
    return sym - '0';
}

uint8_t reverse(uint8_t x) {
    uint8_t result = 0;
    for (uint8_t i = 0; i < 8; ++i) {
        uint8_t cur_bit = x & 1;
        x >>= 1;
        for (uint8_t j = 0; j < 8 - i - 1; ++j) {
            cur_bit <<= 1;
        }
        result |= cur_bit;
    }

    return result;
}

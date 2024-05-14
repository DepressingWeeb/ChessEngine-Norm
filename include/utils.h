#pragma once
#include <stdint.h>

const int index64[64] = {
    0,  1, 48,  2, 57, 49, 28,  3,
   61, 58, 50, 42, 38, 29, 17,  4,
   62, 55, 59, 36, 53, 51, 43, 22,
   45, 39, 33, 30, 24, 18, 12,  5,
   63, 47, 56, 27, 60, 41, 37, 16,
   54, 35, 52, 21, 44, 32, 23, 11,
   46, 26, 40, 15, 34, 20, 31, 10,
   25, 14, 19,  9, 13,  8,  7,  6
};

/**
 * bitScanForward
 * @author Martin Läuter (1997)
 *         Charles E. Leiserson
 *         Harald Prokop
 *         Keith H. Randall
 * "Using de Bruijn Sequences to Index a 1 in a Computer Word"
 * @param bb bitboard to scan
 * @precondition bb != 0
 * @return index (0..63) of least significant one bit
 */

inline int BitScanForward64(uint64_t x) {
    return index64[((x & -x) * 0x03f79d71b4cb0a89ULL) >> 58];
}

inline uint64_t popcount64(uint64_t x) {
    return __popcnt64(x);
}

inline uint32_t popcount32(uint32_t x) {
    return __popcnt(x);
}
/*
//BitScanForward : Search the mask data from least significant bit (LSB) to the most significant bit (MSB) for a set bit (1)
inline int BitScanForward64(uint64_t x) {
    unsigned long index = 0;
    _BitScanForward64(&index,x);
    return index;
}
*/
inline bool BitScanForward32(uint32_t x, unsigned long& index) {
    return _BitScanForward(&index, x);
}

//BitScanReverse: Search the mask data from most significant bit (MSB) to least significant bit (LSB) for a set bit (1).
inline bool BitScanReverse64(uint64_t x, unsigned long& index) {
    return _BitScanReverse64(&index, x);
}

inline bool BitScanReverse32(uint32_t x, unsigned long& index) {
    return _BitScanReverse(&index, x);
}

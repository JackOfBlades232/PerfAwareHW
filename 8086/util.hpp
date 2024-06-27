#pragma once
#include "defs.hpp"
#include <cstdio>
#include <cstring>
#include <cassert>

#define POT(_p) (1 << (_p))

#define ARR_CNT(_arr) (sizeof(_arr) / sizeof((_arr)[0]))

#define LOGERR(_fmt, ...) fprintf(stderr, "[ERR] " _fmt "\n", ##__VA_ARGS__)

template <class T>
inline void swap(T *a, T *b)
{
    T tmp = *a;
    *a = *b;
    *b = tmp;
}

inline u32 n_bit_mask(int nbits)
{
    return (1 << nbits) - 1;
}

inline int count_ones(u32 val, int bits_range)
{
    int res = 0;
    for (u32 mask = 1; mask != (1 << bits_range); mask <<= 1)
        res += (val & mask) ? 1 : 0;
    return res;
}

inline u32 reverse_bits_in_bytes(u32 val, int byte_range)
{
    u32 res = 0;
    int bits_range = byte_range * 8;
    u32 mask = 1;
    for (int i = 0; i < bits_range; ++i, mask <<= 1) {
        u32 src_mask = 1 << ((i/8 + 1)*8 - i%8 - 1);
        res |= (val & src_mask) ? mask : 0;
    }
    return res;
}

inline u32 press_down_masked_bits(u32 val, u32 mask)
{
    u32 res = 0;
    u32 read_mask = 1, write_mask = 1;
    for (int i = 0; i < 16; ++i, read_mask <<= 1) {
        if (mask & read_mask) {
            res |= (read_mask & val) ? write_mask : 0;
            write_mask <<= 1;
        }
    }

    return res;
}

template <class TEnum>
inline u32 to_flag(TEnum e)
{
    assert((int)e < 32);
    return 1 << (int)e;
}

template <class TUint>
inline void set_byte(TUint *dst, u8 val, uint byte_id)
{
    assert(byte_id < sizeof(TUint));
    uint shift = byte_id * 8;
    *dst &= ~(0xFF << shift);
    *dst |= val << shift;
}

inline bool streq(const char *s1, const char *s2)
{
    return strcmp(s1, s2) == 0;
}

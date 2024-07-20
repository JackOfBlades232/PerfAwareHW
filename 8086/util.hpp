#pragma once
#include "defs.hpp"
#include <cstdio>
#include <cstring>
#include <cassert>
#include <bit>

#define POT(p_) (1 << (p_))

#define ARR_CNT(arr_) (sizeof(arr_) / sizeof((arr_)[0]))

#define LOGERR(fmt_, ...) fprintf(stderr, "[ERR] " fmt_ "\n", ##__VA_ARGS__)

#ifndef NDEBUG
  #define ASSERTF(e_, fmt_, ...)           \
      do {                                 \
          if (!e_) {                       \
              LOGERR(fmt_, ##__VA_ARGS__); \
              assert(0);                   \
          }                                \
      } while (0)
#endif

constexpr bool is_little_endian() { return std::endian::native == std::endian::little; }

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

template <class TInt>
inline TInt sgn(TInt n)
{
    return n < 0 ? -1 : (n > 0 ? 1 : 0);
}

template <class TInt>
inline TInt abs(TInt n)
{
    return n * sgn(n);
}

template <class TInt>
inline TInt min(TInt a, TInt b)
{
    return a < b ? a : b;
}

inline void set_flags(u32 *dest, u32 flags, bool set)
{
    if (set)
        *dest |= flags;
    else
        *dest &= ~flags;
}

inline bool streq(const char *s1, const char *s2)
{
    return strcmp(s1, s2) == 0;
}

template <class TFAction>
class DeferredAction__ {
    TFAction m_action;

public:
    DeferredAction__(TFAction &&action) : m_action(action) {}
    ~DeferredAction__() { m_action(); }
};

#define DEFER(action_) DeferredAction__ deferred_##__LINE__(std::move(action_));

#pragma once

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#include <cstdint>
#include <cstddef>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cerrno>
#include <climits>
#include <cfloat>

#include <cmath>
#include <cstring>
#include <initializer_list>

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;
using f32 = float;
using f64 = double;
using b32 = u32;
using uint = unsigned int;
using uchar = unsigned char;
using ushort = unsigned short;
using ulong = unsigned long;
using llong = long long;
using ullong = unsigned long long;

namespace meta
{

template <typename T>
struct RemoveReference { using type_t = T; };
template <typename T>
struct RemoveReference<T &> { using type_t = T; };
template <typename T>
struct RemoveReference<T &&> { using type_t = T; };
template <typename T>
using remove_reference_t = typename RemoveReference<T>::type_t;

template <typename T>
struct MakeSigned;
template <>
struct MakeSigned<uchar> { using type_t = char; };
template <>
struct MakeSigned<ushort> { using type_t = short; };
template <>
struct MakeSigned<uint> { using type_t = int; };
template <>
struct MakeSigned<ulong> { using type_t = long; };
template <>
struct MakeSigned<ullong> { using type_t = long long; };
template <typename T>
using make_signed_t = typename MakeSigned<T>::type_t;

}

#ifdef _WIN32
using ssize_t = meta::make_signed_t<size_t>;
#else
#include <sys/types.h>
#endif

using usize = size_t;
using isize = ssize_t;

#define CAT_(a_, b_) a_ ## b_
#define CAT(a_, b_) CAT_(a_, b_)

#define ARR_CNT(arr_) (sizeof(arr_) / sizeof((arr_)[0]))

template <typename T>
inline constexpr T min(T a, T b)
{
    return a < b ? a : b;
}

template <typename T>
inline constexpr T max(T a, T b)
{
    return a > b ? a : b;
}

template <typename T>
inline constexpr T abs(T a)
{
    return a < T{0} ? -a : a;
}

template <typename TInt, typename TFlt>
inline constexpr TInt round_to_int(TFlt f)
{
    return TInt(f + TFlt{0.50000001});
}

template <typename T>
inline constexpr auto mv(T &&var)
{
    return (meta::remove_reference_t<decltype(var)> &&)(var);
}

template <typename T>
inline constexpr T &&fwd(T &&var)
{
    return (T &&)(var);
}

template <typename T, typename U>
inline constexpr T xchg(T &var, U newval)
{
    T ret = mv(var);
    var = mv(newval);
    return ret;
}

template <typename T>
inline constexpr void swp(T &v1, T &v2)
{
    T tmp = mv(v1);
    v1 = mv(v2);
    v2 = mv(tmp);
}

inline bool streq(const char *s1, const char *s2)
{
    return strcmp(s1, s2) == 0;
}

template <class T>
inline constexpr T round_up(T val, T measure)
{
    return measure * ((val - 1) / measure + 1);
}

template <class T>
inline constexpr T round_down(T val, T measure)
{
    return measure * (val / measure);
}

template <class T> // For higher precision/not being completely out of range
inline constexpr f64 large_divide(T v, T d)
{
    T whole = v / d;
    f64 frac = f64(v - whole * d) / d;
    return f64(whole) + frac;
}

inline constexpr u64 c_bytes_in_gb = 1u << 30;
inline constexpr u64 c_kb_in_gb = 1u << 20;

inline constexpr f64 gb_per_measure(f64 measure, u64 bytes)
{
    f64 const gb = large_divide(bytes, c_bytes_in_gb); 
    return gb / measure;
};

inline constexpr u64 kb(u64 cnt)
{
    return cnt << 10;
}

inline constexpr u64 mb(u64 cnt)
{
    return cnt << 20;
}

inline constexpr u64 gb(u64 cnt)
{
    return cnt << 30;
}

inline constexpr u64 clines(u64 cnt)
{
    return cnt << 6;
}


#pragma once

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#include <cstring>
#include <cstdint>
#include <cstddef>
#include <initializer_list>

#define CAT_(a_, b_) a_ ## b_
#define CAT(a_, b_) CAT_(a_, b_)

#define ARR_CNT(arr_) (sizeof(arr_) / sizeof((arr_)[0]))

struct nil_t {};

namespace meta
{

template <typename T>
struct RemoveReference {
    using Type = T;
};
template <typename T>
struct RemoveReference<T &> {
    using Type = T;
};
template <typename T>
struct RemoveReference<T &&> {
    using Type = T;
};

template <typename T>
using remove_reference_t = typename RemoveReference<T>::Type;

}

#define FOR(arr_)                                                        \
    for (meta::remove_reference_t<decltype(arr_[0])> *it = arr_.Begin(); \
         it != arr_.End();                                               \
         ++it)

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

inline constexpr long double c_bytes_in_gb = (long double)(1u << 30);
inline constexpr long double c_kb_in_gb = (long double)(1u << 20);

inline constexpr long double gb_per_measure(long double measure, uint64_t bytes)
{
    long double const gb = (long double)bytes / c_bytes_in_gb; 
    return gb / measure;
};

inline constexpr uint64_t kb(uint64_t cnt)
{
    return cnt << 10;
}

inline constexpr uint64_t mb(uint64_t cnt)
{
    return cnt << 20;
}

inline constexpr uint64_t gb(uint64_t cnt)
{
    return cnt << 30;
}

inline constexpr uint64_t clines(uint64_t cnt)
{
    return cnt << 6;
}


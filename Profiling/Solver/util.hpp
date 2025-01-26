#pragma once

#include <cstring>

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
inline T min(T a, T b)
{
    return a < b ? a : b;
}

template <typename T>
inline T max(T a, T b)
{
    return a > b ? a : b;
}

template <typename T>
inline auto mv(T &&var)
{
    return (meta::remove_reference_t<decltype(var)> &&)(var);
}

template <typename T>
inline T &&fwd(T &&var)
{
    return (T &&)(var);
}

template <typename T, typename U>
inline T xchg(T &var, U newval)
{
    T ret = mv(var);
    var = mv(newval);
    return ret;
}

template <typename T>
inline void swp(T &v1, T &v2)
{
    T tmp = mv(v1);
    v1 = mv(v2);
    v2 = mv(tmp);
}

inline bool streq(const char *s1, const char *s2)
{
    return strcmp(s1, s2) == 0;
}

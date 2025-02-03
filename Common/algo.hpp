#pragma once

#include <cstring>

// @TODO: value semantics for non-memcopyable & a normal sorting algo

template <class T, class TComp>
void insertion_sort(T *start, T *end, TComp &&comp)
{
    for (T *r = start + 1; r < end; ++r) {
        T *p;
        for (p = r - 1; p >= start; --p) {
            if (comp(*p, *r))
                break;
        }
        if (p < r - 1) {
            T rr = *r;
            memmove(p + 2, p + 1, (r - p - 1) * sizeof(T));
            *(p + 1) = rr;
        }
    }
}


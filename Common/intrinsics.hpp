#pragma once

#if _WIN32

#include <intrin.h>

inline void i_movsb(unsigned char *dst, unsigned char const *src, size_t cnt)
{
    __movsb(dst, src, cnt);
}

#else

#include <x86intrin.h>
#include <immintrin.h>

inline void i_movsb(unsigned char *dst, unsigned char const *src, size_t cnt)
{
    asm volatile(
        "rep movsb" 
        : "+D"(dst), "+S"(src), "+c"(cnt)
        : 
        : "memory");
}

#endif

#pragma once

#include "defs.hpp"

#if _WIN32

#include <intrin.h>

inline void i_movsb(uchar *dst, uchar const *src, usize cnt)
{
    __movsb(dst, src, cnt);
}

inline void i_full_compiler_barrier()
{
    _ReadWriteBarrier(); 
}

#else

#include <x86intrin.h>
#include <immintrin.h>

inline void i_movsb(uchar *dst, uchar const *src, usize cnt)
{
    asm volatile(
        "rep movsb" 
        : "+D"(dst), "+S"(src), "+c"(cnt)
        : 
        : "memory");
}

inline void i_full_compiler_barrier()
{
    asm volatile("" ::: "memory");
}

#endif

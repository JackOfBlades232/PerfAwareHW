#pragma once

#include "defs.hpp"

#if _WIN32

#include <intrin.h>

FINLINE void i_movsb(uchar *dst, uchar const *src, usize cnt)
{
    __movsb(dst, src, cnt);
}

FINLINE void i_full_compiler_barrier()
{
    _ReadWriteBarrier(); 
}

#else

#include <x86intrin.h>
#include <immintrin.h>

FINLINE void i_movsb(uchar *dst, uchar const *src, usize cnt)
{
    asm volatile(
        "rep movsb" 
        : "+D"(dst), "+S"(src), "+c"(cnt)
        : 
        : "memory");
}

FINLINE void i_full_compiler_barrier()
{
    asm volatile("" ::: "memory");
}

#endif


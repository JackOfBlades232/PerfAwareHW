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

inline f64 i_sqrt(f64 in)
{
    __m128d xmm = _mm_set_sd(in);
    __m128d sqrt_xmm = _mm_sqrt_sd(xmm, xmm);
    return _mm_cvtsd_f64(sqrt_xmm);
}

inline f64 i_sqrt_approx_via_downcast(f64 in)
{
    __m128 xmm = _mm_set_ss(f32(in));
    __m128 sqrt_xmm = _mm_sqrt_ss(xmm);
    return f64(_mm_cvtss_f32(sqrt_xmm));
}

inline f64 i_sqrt_approx_via_rsqrt(f64 in)
{
    __m128 xmm = _mm_set_ss(f32(in));
    __m128 rsqrt_xmm = _mm_rsqrt_ss(xmm);
    __m128 sqrt_xmm = _mm_rcp_ss(rsqrt_xmm);
    return f64(_mm_cvtss_f32(sqrt_xmm));
}

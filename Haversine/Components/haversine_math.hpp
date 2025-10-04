#pragma once

#include <defs.hpp>
#include <intrinsics.hpp>

constexpr f64 c_pi = 3.1415926535897932384626433832795028842;

FINLINE f64 sqrt_s(f64 in)
{
    __m128d xmm = _mm_set_sd(in);
    __m128d sqrt_xmm = _mm_sqrt_sd(xmm, xmm);
    return _mm_cvtsd_f64(sqrt_xmm);
}

FINLINE f64 sqrt_dc(f64 in)
{
    __m128 xmm = _mm_set_ss(f32(in));
    __m128 sqrt_xmm = _mm_sqrt_ss(xmm);
    return f64(_mm_cvtss_f32(sqrt_xmm));
}

FINLINE f64 sqrt_approx(f64 in)
{
    __m128 xmm = _mm_set_ss(f32(in));
    __m128 rsqrt_xmm = _mm_rsqrt_ss(xmm);
    __m128 sqrt_xmm = _mm_rcp_ss(rsqrt_xmm);
    return f64(_mm_cvtss_f32(sqrt_xmm));
}

FINLINE f64 sin_quad_approx(f64 in)
{
    constexpr f64 c_a = 4.0 / (c_pi * c_pi);
    constexpr f64 c_b = 4.0 / c_pi;
    return (in >= 0.0 ? -1.0 : 1.0) * c_a * in * in + c_b * in;
}

FINLINE f64 cos_quad_approx(f64 in)
{
    constexpr f64 c_offs = c_pi * 0.5;
    constexpr f64 c_loopback = 2.0 * c_pi;
    f64 adj = in + c_offs;
    return sin_quad_approx(in > c_offs ? adj - c_loopback : adj); 
}

FINLINE f64 sin_quad_approx_intrin(f64 in)
{
    constexpr f64 c_a = 4.0 / (c_pi * c_pi);
    constexpr f64 c_b = 4.0 / c_pi;
    __m128d x = _mm_set_sd(in);
    __m128d x2 = _mm_mul_sd(x, x);
    __m128d zero = _mm_setzero_pd();
    __m128d mask = _mm_cmpge_sd(x, zero);
    __m128d pa = _mm_set_sd(c_a);
    __m128d na = _mm_sub_sd(zero, pa);
    __m128d a = _mm_or_pd(_mm_and_pd(mask, na), _mm_andnot_pd(mask, pa));
    __m128d b = _mm_set_sd(c_b);
    return _mm_cvtsd_f64(_mm_add_sd(_mm_mul_sd(a, x2), _mm_mul_sd(b, x)));
}

FINLINE f64 cos_quad_approx_intrin(f64 in)
{
    constexpr f64 c_offs = c_pi * 0.5;
    constexpr f64 c_loopback = 2.0 * c_pi;
    __m128d xin = _mm_set_sd(in);
    __m128d offs = _mm_set_sd(c_offs);
    __m128d lb = _mm_set_sd(c_loopback);
    __m128d adj = _mm_add_sd(xin, offs);
    __m128d adj_wrapped = _mm_sub_sd(adj, lb);
    __m128d mask = _mm_cmpgt_sd(xin, offs);
    __m128d sinarg = _mm_or_pd(
        _mm_and_pd(mask, adj_wrapped),
        _mm_andnot_pd(mask, adj));
    return sin_quad_approx_intrin(_mm_cvtsd_f64(sinarg)); 
}

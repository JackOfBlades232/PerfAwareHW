#pragma once

#include <defs.hpp>
#include <intrinsics.hpp>

constexpr f64 c_pi = 3.1415926535897932384626433832795028842;
constexpr f64 c_pi_half = c_pi * 0.5;
constexpr f64 c_sin_quad_a = -3.0 / (c_pi * c_pi);
constexpr f64 c_sin_quad_b = 7.0 / (2.0 * c_pi);
constexpr f64 c_sin_seg_size = c_pi * 0.5;

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
    __m128d xin = _mm_set_sd(in);
    __m128d segsz = _mm_set_sd(c_sin_seg_size);
    __m128d xwseg = _mm_div_sd(xin, segsz);
    __m128d xseg = _mm_floor_sd(xwseg, xwseg);
    u64 seg = u64(_mm_cvtsd_si64(xseg));
    f64 inseg = _mm_cvtsd_f64(_mm_sub_sd(xin, _mm_mul_sd(xseg, segsz)));
    f64 ysign = seg & 0b10 ? -1.0 : 1.0;
    f64 x = seg & 0b01 ? c_pi_half - inseg : inseg;
    f64 x2 = x * x; 
    return ysign * (c_sin_quad_a * x2 + c_sin_quad_b * x);
}

FINLINE f64 cos_quad_approx(f64 in)
{
    return sin_quad_approx(in + c_pi_half);
}

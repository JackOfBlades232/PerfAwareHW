#pragma once

#include <defs.hpp>
#include <intrinsics.hpp>

constexpr f64 c_pi = 3.141592653589793238462643383279502884197169399375105820974944592307816406286208998628034825342117067982148086513282306647093844609550582231725359408128481117450284102701938521105559644622948954930382;
constexpr f64 c_pi_half = 1.570796326794896619231321691639751442098584699687552910487472296153908203143104499314017412671058533991074043256641153323546922304775291115862679704064240558725142051350969260552779822311474477465191;
constexpr f64 c_pi_quat = 0.78539816339744830961566084581987572104929234984377645524373614807695410157155224965700870633552926699553702162832057666177346115238764555793133985203212027936257102567548463027638991115573723873259549;

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

// -3.0 / (c_pi * c_pi)
constexpr f64 c_sin_quad_a = -0.30396355092701331433163838962918291671307632401673964653682709568251936288670632357362782177686551284086673328455715874504218580295825523720801065449044977915856290581184826954827935928637383859959077;
// 7.0 / (2.0 * c_pi)
constexpr f64 c_sin_quad_b = 1.1140846016432673503821863436076005342412175201831951412336714084122775834395857456307966193637716016925098990873057062582503829228020505701457718751218988746046797685618635430115442247784771538210218;
constexpr f64 c_sin_seg_size = c_pi_half;

FINLINE f64 sin_range_reduction(f64 in, auto &&calc)
{
    __m128d xin = _mm_set_sd(in);
    __m128d segsz = _mm_set_sd(c_sin_seg_size);
    __m128d xwseg = _mm_div_sd(xin, segsz);
    __m128d xseg = _mm_floor_sd(xwseg, xwseg);
    u64 seg = u64(_mm_cvtsd_si64(xseg));
    f64 inseg = _mm_cvtsd_f64(_mm_sub_sd(xin, _mm_mul_sd(xseg, segsz)));
    f64 ysign = seg & 0b10 ? -1.0 : 1.0;
    f64 x = seg & 0b01 ? c_pi_half - inseg : inseg;
    return ysign * calc(x);
}

FINLINE f64 sin_quad_approx(f64 in)
{
    return sin_range_reduction(in,
        [](f64 x) { return c_sin_quad_a * x * x + c_sin_quad_b * x; });
}

FINLINE f64 cos_quad_approx(f64 in)
{
    return sin_quad_approx(in + c_pi_half);
}

template <usize t_n>
struct SineTaylorSeriesCoeffs {
    f64 cs[t_n + 1];
};

template <f64 t_point, usize t_n>
inline SineTaylorSeriesCoeffs<t_n> calc_sine_taylor_coeffs()
{
    SineTaylorSeriesCoeffs<t_n> res{};
    res.cs[0] = sin(t_point);

    auto derivative = [](usize n) {
        switch (n & 0b11) {
        case 0:
            return sin(t_point);
        case 1:
            return cos(t_point);
        case 2:
            return -sin(t_point);
        case 3:
            return -cos(t_point);
        default:
            return 0.0;
        }
    };

    u64 factorial_accum = 1;
    for (usize i = 1; i <= t_n; ++i) {
        factorial_accum *= i;
        res.cs[i] = derivative(i) / f64(factorial_accum);
    }

    return res;
}

template <f64 t_point, usize t_n>
inline SineTaylorSeriesCoeffs<t_n> const c_sine_taylor_series_coeffs =
    calc_sine_taylor_coeffs<t_point, t_n>();

template <f64 t_point, usize t_n>
inline void fill_taylor_series_degrees(f64 x, f64 *coeffs)
{
    f64 base = x - t_point;
    f64 next = base;
    for (usize i = 0; i < t_n; ++i) {
        coeffs[i] = next;
        next *= base;
    }
}

template <usize t_n>
FINLINE f64 sin_taylor_approx(f64 in)
{
    return sin_range_reduction(in,
        [](f64 x) {
            auto const &coeffs = c_sine_taylor_series_coeffs<c_pi_quat, t_n>;
            f64 args[t_n];
            fill_taylor_series_degrees<c_pi_quat, t_n>(x, args);
            f64 res = coeffs.cs[0];
            for (usize i = 0; i < t_n; ++i)
                res += args[i] * coeffs.cs[i + 1];
            return res;
        });
}

template <usize t_n>
FINLINE f64 cos_taylor_approx(f64 in)
{
    return sin_taylor_approx<t_n>(in + c_pi_half);
}

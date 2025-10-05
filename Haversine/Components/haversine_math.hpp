#pragma once

#include <defs.hpp>
#include <intrinsics.hpp>

constexpr f64 c_pi64 = 3.141592653589793238462643383279502884197169399375105820974944592307816406286208998628034825342117067982148086513282306647093844609550582231725359408128481117450284102701938521105559644622948954930382;
constexpr f64 c_pi_half64 = 1.570796326794896619231321691639751442098584699687552910487472296153908203143104499314017412671058533991074043256641153323546922304775291115862679704064240558725142051350969260552779822311474477465191;
constexpr f64 c_2pi64 = 6.2831853071795864769252867665590057683943387987502116419498891846156328125724179972560696506842341359642961730265646132941876892191011644634507188162569622349005682054038770422111192892458979098607639;
constexpr f64 c_1oversqrt2_64 = 0.70710678118654752440084436210484903928483593768847403658833986899536623923105351942519376716382078636750692311545614851246241802792536860632206074854996791570661133296375279637789997525057639103028574;
constexpr f64 c_earth_rad64 = 6378.1;

FINLINE __m128d bool2mask_sd(bool b)
{
    u64 mask = u64(-i64(b));
    return _mm_castsi128_pd(_mm_set_epi64x(mask, mask));
}

FINLINE f64 sqrt_e(f64 in)
{
    __m128d xmm = _mm_set_sd(in);
    __m128d sqrt_xmm = _mm_sqrt_sd(xmm, xmm);
    return _mm_cvtsd_f64(sqrt_xmm);
}

FINLINE f64 sin_a(f64 in)
{
    auto ternary = [](bool cond, f64 l, f64 r) {
        return _mm_blendv_pd(_mm_set_sd(r), _mm_set_sd(l), bool2mask_sd(cond));
    };

    __m128d xin = _mm_set_sd(in);
    __m128d ysign = ternary(in < 0.0, -1.0, 1.0);
    f64 inseg = _mm_cvtsd_f64(_mm_mul_sd(xin, ysign));
    __m128d x = ternary(inseg > c_pi_half64, c_pi64 - inseg, inseg);
    __m128d x2 = _mm_mul_sd(x, x);

    __m128d r = _mm_set_sd(0x1.883c1c5deffbep-49);
    r = _mm_fmadd_sd(r, x2, _mm_set_sd(-0x1.ae43dc9bf8ba7p-41));
    r = _mm_fmadd_sd(r, x2, _mm_set_sd(0x1.6123ce513b09fp-33));
    r = _mm_fmadd_sd(r, x2, _mm_set_sd(-0x1.ae6454d960ac4p-26));
    r = _mm_fmadd_sd(r, x2, _mm_set_sd(0x1.71de3a52aab96p-19));
    r = _mm_fmadd_sd(r, x2, _mm_set_sd(-0x1.a01a01a014eb6p-13));
    r = _mm_fmadd_sd(r, x2, _mm_set_sd(0x1.11111111110c9p-7));
    r = _mm_fmadd_sd(r, x2, _mm_set_sd(-0x1.5555555555555p-3));
    r = _mm_fmadd_sd(r, x2, _mm_set_sd(0x1p0));
    r = _mm_mul_sd(r, x);

    return _mm_cvtsd_f64(_mm_mul_sd(ysign, r));
}

FINLINE f64 cos_a(f64 in)
{
    return sin_a(in + c_pi_half64);
}

FINLINE f64 asin_a(f64 in)
{
    __m128d cvt_mask = bool2mask_sd(in > c_1oversqrt2_64);

    __m128d insin = _mm_set_sd(in);
    __m128d insin2 = _mm_mul_sd(insin, insin);
    __m128d one = _mm_set_sd(1.0);
    __m128d incos2 = _mm_sub_sd(one, insin2);
    __m128d incos = _mm_sqrt_sd(incos2, incos2);

    __m128d x = _mm_blendv_pd(insin, incos, cvt_mask);
    __m128d x2 = _mm_blendv_pd(insin2, incos2, cvt_mask);

    __m128d r = _mm_set_sd(0x1.7f820d52c2775p-1);
    r = _mm_fmadd_sd(r, x2, _mm_set_sd(-0x1.4d84801ff1aa1p1));
    r = _mm_fmadd_sd(r, x2, _mm_set_sd(0x1.14672d35db97ep2));
    r = _mm_fmadd_sd(r, x2, _mm_set_sd(-0x1.188f223fe5f34p2));
    r = _mm_fmadd_sd(r, x2, _mm_set_sd(0x1.86bbff2a6c7b6p1));
    r = _mm_fmadd_sd(r, x2, _mm_set_sd(-0x1.83633c76e4551p0));
    r = _mm_fmadd_sd(r, x2, _mm_set_sd(0x1.224c4dbe13cbdp-1));
    r = _mm_fmadd_sd(r, x2, _mm_set_sd(-0x1.2ab04ba9012e3p-3));
    r = _mm_fmadd_sd(r, x2, _mm_set_sd(0x1.5565a3d3908b9p-5));
    r = _mm_fmadd_sd(r, x2, _mm_set_sd(0x1.b1b8d27cd7e72p-8));
    r = _mm_fmadd_sd(r, x2, _mm_set_sd(0x1.dc086c5d99cdcp-7));
    r = _mm_fmadd_sd(r, x2, _mm_set_sd(0x1.1b8cc838ee86ep-6));
    r = _mm_fmadd_sd(r, x2, _mm_set_sd(0x1.6e96be6dbe49ep-6));
    r = _mm_fmadd_sd(r, x2, _mm_set_sd(0x1.f1c6b0ea300d7p-6));
    r = _mm_fmadd_sd(r, x2, _mm_set_sd(0x1.6db6dca9f82d4p-5));
    r = _mm_fmadd_sd(r, x2, _mm_set_sd(0x1.3333333148aa7p-4));
    r = _mm_fmadd_sd(r, x2, _mm_set_sd(0x1.555555555683fp-3));
    r = _mm_fmadd_sd(r, x2, _mm_set_sd(0x1.fffffffffffffp-1));
    r = _mm_mul_sd(r, x);

    __m128d cr = _mm_sub_sd(_mm_set_sd(c_pi_half64), r);
    return _mm_cvtsd_f64(_mm_blendv_pd(r, cr, cvt_mask));
}

// Range test stuff
struct function_input_range_t {
    f64 min = DBL_MAX, max = -DBL_MAX;
};

inline void update(function_input_range_t &rng, f64 val)
{
    rng.min = min(rng.min, val);
    rng.max = max(rng.max, val);
}

struct haversine_function_input_ranges_t {
    function_input_range_t cos_input_range = {};
    function_input_range_t asin_input_range = {};
    function_input_range_t sqrt_input_range = {};
};

inline f64 range_cos(f64 a, void *rng)
{
    update(((haversine_function_input_ranges_t *)rng)->cos_input_range, a);
    return cos(a);
}

inline f64 range_asin(f64 a, void *rng)
{
    update(((haversine_function_input_ranges_t *)rng)->asin_input_range, a);
    return asin(a);
}

inline f64 range_sqrt(f64 a, void *rng)
{
    update(((haversine_function_input_ranges_t *)rng)->sqrt_input_range, a);
    return sqrt(a);
}

#pragma once

#include <defs.hpp>
#include <intrinsics.hpp>

constexpr f64 c_pi = 3.141592653589793238462643383279502884197169399375105820974944592307816406286208998628034825342117067982148086513282306647093844609550582231725359408128481117450284102701938521105559644622948954930382;
constexpr f64 c_pi_half = 1.570796326794896619231321691639751442098584699687552910487472296153908203143104499314017412671058533991074043256641153323546922304775291115862679704064240558725142051350969260552779822311474477465191;
constexpr f64 c_1oversqrt2 = 0.70710678118654752440084436210484903928483593768847403658833986899536623923105351942519376716382078636750692311545614851246241802792536860632206074854996791570661133296375279637789997525057639103028574;

FINLINE f64 sqrt_e(f64 in)
{
    __m128d xmm = _mm_set_sd(in);
    __m128d sqrt_xmm = _mm_sqrt_sd(xmm, xmm);
    return _mm_cvtsd_f64(sqrt_xmm);
}

FINLINE f64 sin_a(f64 in)
{
    auto ternary = [](u64 cond, f64 l, f64 r) {
        u64 mask = u64(-i64(cond != 0));
        __m128d xmm_mask = _mm_castsi128_pd(_mm_set_epi64x(mask, mask));
        return _mm_blendv_pd(_mm_set_sd(r), _mm_set_sd(l), xmm_mask);
    };

    __m128d xin = _mm_set_sd(in);
    __m128d segsz = _mm_set_sd(c_pi_half);
    __m128d xwseg = _mm_div_sd(xin, segsz);
    __m128d xseg = _mm_floor_sd(xwseg, xwseg);
    u64 seg = u64(_mm_cvtsd_si64(xseg));
    f64 inseg = _mm_cvtsd_f64(_mm_sub_sd(xin, _mm_mul_sd(xseg, segsz)));
    __m128d ysign = ternary(seg & 0b10, -1.0, 1.0);
    __m128d x = ternary(seg & 0b01, c_pi_half - inseg, inseg);
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

FINLINE f64 asin_a(f64 in)
{
    bool cvt = in > c_1oversqrt2;
    u64 cvt_mask_u = u64(-i64(cvt));
    __m128d cvt_mask = _mm_castsi128_pd(_mm_set_epi64x(cvt_mask_u, cvt_mask_u));

    __m128d insin = _mm_set_sd(in);
    __m128d insin2 = _mm_mul_sd(insin, insin);
    __m128d one = _mm_set_sd(1.0);
    __m128d incos2 = _mm_sub_sd(one, insin2);
    // @TODO: chosen sqrt here
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

    __m128d cr = _mm_sub_sd(_mm_set_sd(c_pi_half), r);
    return _mm_cvtsd_f64(_mm_blendv_pd(r, cr, cvt_mask));
}

FINLINE f64 cos_a(f64 in)
{
    return sin_a(in + c_pi_half);
}

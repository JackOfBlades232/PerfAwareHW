#pragma once

#include <defs.hpp>
#include <intrinsics.hpp>

constexpr f64 c_pi = 3.141592653589793238462643383279502884197169399375105820974944592307816406286208998628034825342117067982148086513282306647093844609550582231725359408128481117450284102701938521105559644622948954930382;
constexpr f64 c_pi_half = 1.570796326794896619231321691639751442098584699687552910487472296153908203143104499314017412671058533991074043256641153323546922304775291115862679704064240558725142051350969260552779822311474477465191;

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

FINLINE f64 cos_a(f64 in)
{
    return sin_a(in + c_pi_half);
}

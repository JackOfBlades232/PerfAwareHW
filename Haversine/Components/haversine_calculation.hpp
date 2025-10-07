#pragma once

#include "haversine_common.hpp"
#include "haversine_state.hpp"
#include "haversine_math.hpp"

#include <defs.hpp>

inline f64 haversine_dist_naive_templ(
    point_pair_t pair,
    auto &&cosfunc, auto &&asinfunc, auto &&sqrtfunc,
    void *user)
{
    auto deg2rad = [](f64 deg) { return deg * c_pi64 / 180.0; };
    auto cvtraddist = [](f64 rd) {
        return abs(rd) > c_pi64 ? -sgn(rd) * (c_2pi64 - abs(rd)) : rd;
    };

    f64 dx = cvtraddist(deg2rad(pair.x0 - pair.x1));
    f64 dy = cvtraddist(deg2rad(pair.y0 - pair.y1));

    f64 c1 = cosfunc(dx, user);
    f64 c2 = cosfunc(deg2rad(pair.x0), user);
    f64 c3 = cosfunc(deg2rad(pair.x1), user);
    f64 c4 = cosfunc(dy, user);
    f64 nom =
        1.0 - c1 + (c2 * c3 * (1.0 - c4));
    f64 angle = asinfunc(sqrtfunc(nom * 0.5, user), user);
    return 2.0 * c_earth_rad64 * angle;
}

inline f64 haversine_dist_naive(point_pair_t pair)
{
    return haversine_dist_naive_templ(
        pair,
        [](f64 a, void *) { return cos(a); },
        [](f64 a, void *) { return asin(a); },
        [](f64 a, void *) { return sqrt(a); },
        nullptr);
}

inline f64 haversine_dist_range_check(
    point_pair_t pair, haversine_function_input_ranges_t &rngs)
{
    return haversine_dist_naive_templ(
        pair, &range_cos, &range_asin, &range_sqrt, &rngs);
}

inline f64 haversine_dist_our_funcs(point_pair_t pair)
{
    return haversine_dist_naive_templ(
        pair,
        [](f64 a, void *) { return cos_a(a); },
        [](f64 a, void *) { return asin_a(a); },
        [](f64 a, void *) { return sqrt_e(a); },
        nullptr);
}

inline void calculate_haversine_distances(
    haversine_state_t &s, auto &&calculator)
{
    PROFILED_BANDWIDTH_FUNCTION_PF(s.pair_cnt * sizeof(point_pair_t));

    s.sum_answer = 0.0;

    for (u32 i = 0; i < s.pair_cnt; ++i) {
        f64 const dist = calculator(s.pairs[i]);
        s.answers[i] = dist;
        s.sum_answer += dist;
    }
}

inline void calculate_haversine_distances_naive(haversine_state_t &s)
{
    calculate_haversine_distances(s, &haversine_dist_naive);
}

inline void calculate_haversine_distances_our_funcs(haversine_state_t &s)
{
    calculate_haversine_distances(s, &haversine_dist_our_funcs);
}

inline f64 haversine_dist_reference(point_pair_t pair)
{
    return haversine_dist_naive(pair);
}

inline void calculate_haversine_distances_inline(haversine_state_t &s)
{
    PROFILED_BANDWIDTH_FUNCTION_PF(s.pair_cnt * sizeof(point_pair_t));

    constexpr f64 c_deg2rad = c_pi64 / 180.0;
    constexpr f64 c_rad_coeff = c_earth_rad64 * 2.0;

    s.sum_answer = 0.0;

    point_pair_t const *src = s.pairs;
    f64 *dst = s.answers;

    // @TODO: make a proper helper func? Can't decide on return types yet.
    auto ternary = [](bool cond, f64 l, f64 r) {
        return _mm_cvtsd_f64(_mm_blendv_pd(
            _mm_set_sd(r), _mm_set_sd(l), bool2mask_sd(cond)));
    };

    for (u32 i = 0; i < s.pair_cnt; ++i, ++src) {
        f64 xr0 = c_deg2rad * src->x0 + c_pi_half64;
        f64 xr1 = c_deg2rad * src->x1 + c_pi_half64;
        f64 yr0 = c_deg2rad * src->y0;
        f64 yr1 = c_deg2rad * src->y1;

        f64 dxr = xr0 - xr1;
        f64 dyr = yr0 - yr1;
        f64 adxr = ternary(dxr < 0.0, -dxr, dxr);
        f64 dx = ternary(adxr > c_pi64, c_2pi64 - adxr, adxr);
        f64 dy = ternary(dyr < 0.0, -dyr, dyr); // Already from -pi to pi

        __m256d a = _mm256_set_pd(
            c_pi_half64 - dy,
            c_pi_half64 - dx,
            ternary(xr1 > c_pi_half64, c_pi64 - xr1, xr1),
            ternary(xr0 > c_pi_half64, c_pi64 - xr0, xr0));
        __m256d a2 = _mm256_mul_pd(a, a);
        __m256d cosines = _mm256_set1_pd(0x1.883c1c5deffbep-49);
        cosines = _mm256_fmadd_pd(cosines, a2, _mm256_set1_pd(-0x1.ae43dc9bf8ba7p-41));
        cosines = _mm256_fmadd_pd(cosines, a2, _mm256_set1_pd(0x1.6123ce513b09fp-33));
        cosines = _mm256_fmadd_pd(cosines, a2, _mm256_set1_pd(-0x1.ae6454d960ac4p-26));
        cosines = _mm256_fmadd_pd(cosines, a2, _mm256_set1_pd(0x1.71de3a52aab96p-19));
        cosines = _mm256_fmadd_pd(cosines, a2, _mm256_set1_pd(-0x1.a01a01a014eb6p-13));
        cosines = _mm256_fmadd_pd(cosines, a2, _mm256_set1_pd(0x1.11111111110c9p-7));
        cosines = _mm256_fmadd_pd(cosines, a2, _mm256_set1_pd(-0x1.5555555555555p-3));
        cosines = _mm256_fmadd_pd(cosines, a2, _mm256_set1_pd(0x1p0));
        cosines = _mm256_mul_pd(cosines, a);

        f64 cosx0 = _mm256_cvtsd_f64(cosines);
        f64 cosx1 = _mm256_cvtsd_f64(_mm256_permute4x64_pd(cosines, 0b01010101));
        f64 cosdx = _mm256_cvtsd_f64(_mm256_permute4x64_pd(cosines, 0b10101010));
        f64 cosdy = _mm256_cvtsd_f64(_mm256_permute4x64_pd(cosines, 0b11111111));

        f64 cosx0_cosx1 = cosx0 * cosx1;
        f64 m1_cosdy = 1.0 - cosdy;
        f64 mterm = cosx0_cosx1 * m1_cosdy;
        f64 sterm = 1.0 - cosdx + mterm;

        f64 hsterm = sterm * 0.5;

        __m128d cvt_mask = bool2mask_sd(hsterm > 0.5);
        __m128d angle_x2 = _mm_blendv_pd(
            _mm_set_sd(hsterm), _mm_set_sd(1.0 - hsterm), cvt_mask);
        __m128d angle_x = _mm_sqrt_sd(angle_x2, angle_x2);
        __m128d angle_r = _mm_set_sd(0x1.7f820d52c2775p-1);
        angle_r = _mm_fmadd_sd(angle_r, angle_x2, _mm_set_sd(-0x1.4d84801ff1aa1p1));
        angle_r = _mm_fmadd_sd(angle_r, angle_x2, _mm_set_sd(0x1.14672d35db97ep2));
        angle_r = _mm_fmadd_sd(angle_r, angle_x2, _mm_set_sd(-0x1.188f223fe5f34p2));
        angle_r = _mm_fmadd_sd(angle_r, angle_x2, _mm_set_sd(0x1.86bbff2a6c7b6p1));
        angle_r = _mm_fmadd_sd(angle_r, angle_x2, _mm_set_sd(-0x1.83633c76e4551p0));
        angle_r = _mm_fmadd_sd(angle_r, angle_x2, _mm_set_sd(0x1.224c4dbe13cbdp-1));
        angle_r = _mm_fmadd_sd(angle_r, angle_x2, _mm_set_sd(-0x1.2ab04ba9012e3p-3));
        angle_r = _mm_fmadd_sd(angle_r, angle_x2, _mm_set_sd(0x1.5565a3d3908b9p-5));
        angle_r = _mm_fmadd_sd(angle_r, angle_x2, _mm_set_sd(0x1.b1b8d27cd7e72p-8));
        angle_r = _mm_fmadd_sd(angle_r, angle_x2, _mm_set_sd(0x1.dc086c5d99cdcp-7));
        angle_r = _mm_fmadd_sd(angle_r, angle_x2, _mm_set_sd(0x1.1b8cc838ee86ep-6));
        angle_r = _mm_fmadd_sd(angle_r, angle_x2, _mm_set_sd(0x1.6e96be6dbe49ep-6));
        angle_r = _mm_fmadd_sd(angle_r, angle_x2, _mm_set_sd(0x1.f1c6b0ea300d7p-6));
        angle_r = _mm_fmadd_sd(angle_r, angle_x2, _mm_set_sd(0x1.6db6dca9f82d4p-5));
        angle_r = _mm_fmadd_sd(angle_r, angle_x2, _mm_set_sd(0x1.3333333148aa7p-4));
        angle_r = _mm_fmadd_sd(angle_r, angle_x2, _mm_set_sd(0x1.555555555683fp-3));
        angle_r = _mm_fmadd_sd(angle_r, angle_x2, _mm_set_sd(0x1.fffffffffffffp-1));
        angle_r = _mm_mul_sd(angle_r, angle_x);
        __m128d angle = _mm_blendv_pd(
            angle_r, _mm_sub_sd(_mm_set_sd(c_pi_half64), angle_r), cvt_mask);

        f64 dist = c_rad_coeff * _mm_cvtsd_f64(angle);

        *dst++ = dist;
        s.sum_answer += dist;
    }
}

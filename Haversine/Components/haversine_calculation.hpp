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

    f64 nom =
        1.0 - cosfunc(dx, user) +
        (
            cosfunc(deg2rad(pair.x0), user) *
            cosfunc(deg2rad(pair.x1), user) *
            (1.0 - cosfunc(dy, user))
        );
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

    for (u32 i = 0; i < s.pair_cnt; ++i, ++src) {
        auto cvtraddist = [](f64 rd) {
            return abs(rd) > c_pi64 ? -sgn(rd) * (c_2pi64 - abs(rd)) : rd;
        };

        f64 xr0 = c_deg2rad * src->x0;
        f64 xr1 = c_deg2rad * src->x1;
        f64 yr0 = c_deg2rad * src->y0;
        f64 yr1 = c_deg2rad * src->y1;

        f64 dx = cvtraddist(xr0 - xr1);
        f64 dy = cvtraddist(yr0 - yr1);

        f64 cos_x0 = cos_a(dx);
        f64 cos_x1 = cos_a(dx);
        f64 cos_dx = cos_a(dx);
        f64 cos_dy = cos_a(dy);

        f64 cosx0_cosx1 = cos_x0 * cos_x1;
        f64 m1_cosdy = 1.0 - cos_dy;
        f64 mterm = cosx0_cosx1 * m1_cosdy;

        f64 sterm = 1.0 - cos_dx + mterm;
        f64 hsterm = sterm * 0.5;

        f64 sqrt_hsterm = sqrt_e(hsterm);
        f64 angle = asin_a(sqrt_hsterm);

        f64 dist = c_rad_coeff * angle;

        *dst++ = dist;
        s.sum_answer += dist;
    }
}

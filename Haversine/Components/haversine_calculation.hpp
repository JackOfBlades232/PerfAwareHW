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
    PROFILED_FUNCTION;

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

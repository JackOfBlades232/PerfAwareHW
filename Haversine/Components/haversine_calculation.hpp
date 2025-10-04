#pragma once

#include "haversine_common.hpp"
#include "haversine_state.hpp"
#include "haversine_math.hpp"

#include <defs.hpp>

constexpr f64 c_earth_rad = 6378.1;

// @TODO: hoist to common?
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

inline f64 haversine_dist_naive_templ(
    point_pair_t pair,
    auto &&cosfunc, auto &&asinfunc, auto &&sqrtfunc,
    void *user)
{
    PROFILED_FUNCTION;

    auto deg2rad = [](f64 deg) { return deg * c_pi / 180.0; };

    f64 dx = deg2rad(pair.x0 - pair.x1);
    f64 dy = deg2rad(pair.y0 - pair.y1);

    f64 nom =
        1.0 - cosfunc(dx, user) +
        (
            cosfunc(deg2rad(pair.x0), user) *
            cosfunc(deg2rad(pair.x1), user) *
            (1.0 - cosfunc(dy, user))
        );
    f64 angle = asinfunc(sqrtfunc(nom * 0.5, user), user);
    return 2.0 * c_earth_rad * angle;
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

inline f64 haversine_dist_reference(point_pair_t pair)
{
    return haversine_dist_naive(pair);
}

#pragma once

#include "haversine_common.hpp"
#include "haversine_state.hpp"

#include <defs.hpp>

constexpr f32 c_pi = 3.14159265359f;
constexpr f32 c_earth_rad = 6378.1f;

// @TODO: hoist to common?
struct function_input_range_t {
    f32 min = FLT_MAX, max = -FLT_MAX;
};

inline void update(function_input_range_t &rng, f32 val)
{
    rng.min = min(rng.min, val);
    rng.max = max(rng.max, val);
}

struct haversine_function_input_ranges_t {
    function_input_range_t cos_input_range = {};
    function_input_range_t asin_input_range = {};
    function_input_range_t sqrt_input_range = {};
};

inline f32 range_cos(f32 a, void *rng)
{
    update(((haversine_function_input_ranges_t *)rng)->cos_input_range, a);
    return cosf(a);
}

inline f32 range_asin(f32 a, void *rng)
{
    update(((haversine_function_input_ranges_t *)rng)->asin_input_range, a);
    return asinf(a);
}

inline f32 range_sqrt(f32 a, void *rng)
{
    update(((haversine_function_input_ranges_t *)rng)->sqrt_input_range, a);
    return sqrtf(a);
}

inline f32 haversine_dist_naive_templ(
    point_pair_t pair,
    auto &&cosfunc, auto &&asinfunc, auto &&sqrtfunc,
    void *user)
{
    PROFILED_FUNCTION;

    auto deg2rad = [](f32 deg) { return deg * c_pi / 180.f; };

    f32 dx = deg2rad(pair.x0 - pair.x1);
    f32 dy = deg2rad(pair.y0 - pair.y1);

    f32 nom =
        1.f - cosfunc(dx, user) +
        (
            cosfunc(deg2rad(pair.x0), user) *
            cosfunc(deg2rad(pair.x1), user) *
            (1.f - cosfunc(dy, user))
        );
    f32 angle = asinfunc(sqrtfunc(nom * 0.5f, user), user);
    return 2.f * c_earth_rad * angle;
}

inline f32 haversine_dist_naive(point_pair_t pair)
{
    return haversine_dist_naive_templ(
        pair,
        [](f32 a, void *) { return cosf(a); },
        [](f32 a, void *) { return asinf(a); },
        [](f32 a, void *) { return sqrtf(a); },
        nullptr);
}

inline f32 haversine_dist_range_check(
    point_pair_t pair, haversine_function_input_ranges_t &rngs)
{
    return haversine_dist_naive_templ(
        pair, &range_cos, &range_asin, &range_sqrt, &rngs);
}

inline void calculate_haversine_distances(
    haversine_state_t &s, auto &&calculator)
{
    PROFILED_BANDWIDTH_FUNCTION_PF(s.pair_cnt * sizeof(point_pair_t));

    s.sum_answer = 0.f;

    for (u32 i = 0; i < s.pair_cnt; ++i) {
        f32 const dist = calculator(s.pairs[i]);
        s.answers[i] = dist;
        s.sum_answer += dist;
    }
}

inline void calculate_haversine_distances_naive(haversine_state_t &s)
{
    calculate_haversine_distances(s, &haversine_dist_naive);
}

inline f32 haversine_dist_reference(point_pair_t pair)
{
    return haversine_dist_naive(pair);
}

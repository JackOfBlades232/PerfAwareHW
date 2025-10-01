#pragma once

#include "haversine_common.hpp"

inline f32 haversine_dist_naive(point_pair_t pair)
{
    PROFILED_FUNCTION;

    constexpr f32 c_pi = 3.14159265359f;

    auto deg2rad = [](f32 deg) { return deg * c_pi / 180.f; };
    auto rad2deg = [](f32 rad) { return rad * 180.f / c_pi; };

    auto geod = [](f32 rad) { return rad > c_pi ? 2.f * c_pi - rad : rad; };
    auto haversine = [](f32 rad) { return (1.f - cosf(rad)) * 0.5f; };

    f32 dx = geod(deg2rad(fabsf(pair.x0 - pair.x1)));
    f32 dy = geod(deg2rad(fabsf(pair.y0 - pair.y1)));
    f32 y0r = deg2rad(pair.y0);
    f32 y1r = deg2rad(pair.y1);

    f32 hav_of_diff =
        haversine(dy) + cosf(y0r)*cosf(y1r)*haversine(dx);

    f32 rad_of_diff = acosf(1.f - 2.f*hav_of_diff);
    return rad2deg(rad_of_diff);
}

inline void calculate_haversine_distances(
    haversine_state_t &s, auto &&calculator)
{
    PROFILED_BANDWIDTH_FUNCTION_PF(s.pair_cnt * sizeof(point_pair_t));

    s.sum_answer = 0.f;

    for (u32 i = 0; i < s.pair_cnt; ++i) {
        f32 const dist = haversine_dist_naive(s.pairs[i]);
        s.answers[i] = dist;
        s.sum_answer += dist;
    }
}

inline void calculate_haversine_distances_naive(haversine_state_t &s)
{
    calculate_haversine_distances(s, &haversine_dist_naive);
}

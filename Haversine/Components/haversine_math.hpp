#pragma once

#include <defs.hpp>
#include <intrinsics.hpp>

constexpr f64 c_pi = 3.1415926535897932384626433832795028842;

FINLINE f64 sin_quad_approx(f64 in)
{
    constexpr f64 c_a = 4.0 / (c_pi * c_pi);
    constexpr f64 c_b = 4.0 / c_pi;
    return (in >= 0.0 ? -1.0 : 1.0) * c_a * in * in + c_b * in;
}

FINLINE f64 cos_quad_approx(f64 in)
{
    constexpr f64 c_offs = c_pi * 0.5;
    constexpr f64 c_loopback = 2.0 * c_pi;
    f64 adj = in + c_offs;
    return sin_quad_approx(in > c_offs ? adj - c_loopback : adj); 
}

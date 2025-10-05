#include <haversine_math.hpp>

#include <os.hpp>
#include <logging.hpp>

struct func_test_t {
    f64 in;
    f64 out;
};

constexpr func_test_t c_cosine_gold_standard[] =
{
    {0.0000000000000000000000000000000000000, 1.0000000000000000000000000000000000000},
    {0.3490658503988659153847381536977228602, 0.9396926207859083840541092773247314699}, // pi / 9
    {0.5235987755982988730771072305465838140, 0.8660254037844386467637231707529361835}, // pi / 6
    {0.7853981633974483096156608458198757210, 0.7071067811865475244008443621048490393}, // pi / 4
    {1.0471975511965977461542144610931676281, 0.5000000000000000000000000000000000000}, // pi / 3
    {1.5707963267948966192313216916397514421, 0.0000000000000000000000000000000000000}, // pi / 2
    {2.0943951023931954923084289221863352562, -0.5000000000000000000000000000000000000}, // 2pi / 3
    {2.3561944901923449288469825374596271631, -0.7071067811865475244008443621048490393}, // 3pi / 4
    {2.6179938779914943643855361527329190701, -0.8660254037844386467637231707529361835}, // 5pi / 6
    {3.1415926535897932384626433832795028842, -1.0000000000000000000000000000000000000}, // pi
};

constexpr func_test_t c_arcsine_gold_standard[] =
{
    {-1.0000000000000000000000000000000000000, -1.5707963267948966192313216916397514421}, // -pi/2
    {-0.8660254037844386467637231707529361835, -1.0471975511965977461542144610931676281}, // -sqrt(3)/2 -> -pi/3
    {-0.7071067811865475244008443621048490393, -0.7853981633974483096156608458198757210}, // -sqrt(2)/2 -> -pi/4
    {-0.5000000000000000000000000000000000000, -0.5235987755982988730771072305465838140}, // -pi/6
    { 0.0000000000000000000000000000000000000,  0.0000000000000000000000000000000000000}, // 0
    { 0.5000000000000000000000000000000000000,  0.5235987755982988730771072305465838140}, // pi/6
    { 0.7071067811865475244008443621048490393,  0.7853981633974483096156608458198757210}, // sqrt(2)/2 -> pi/4
    { 0.8660254037844386467637231707529361835,  1.0471975511965977461542144610931676281}, // sqrt(3)/2 -> pi/3
    { 1.0000000000000000000000000000000000000,  1.5707963267948966192313216916397514421}, // pi/2
};

constexpr func_test_t c_sqrt_gold_standard[] =
{
    {0.0000000000000000000000000000000000000, 0.0000000000000000000000000000000000000}, // sqrt(0) = 0
    {1.0000000000000000000000000000000000000, 1.0000000000000000000000000000000000000}, // sqrt(1) = 1
    {2.0000000000000000000000000000000000000, 1.4142135623730950488016887242096980786}, // sqrt(2)
    {3.0000000000000000000000000000000000000, 1.7320508075688772935274463415058723669}, // sqrt(3)
    {4.0000000000000000000000000000000000000, 2.0000000000000000000000000000000000000}, // sqrt(4) = 2
    {5.0000000000000000000000000000000000000, 2.2360679774997896964091736687312762354}, // sqrt(5)
    {9.0000000000000000000000000000000000000, 3.0000000000000000000000000000000000000}, // sqrt(9) = 3
    {16.000000000000000000000000000000000000, 4.0000000000000000000000000000000000000}, // sqrt(16) = 4
    {25.000000000000000000000000000000000000, 5.0000000000000000000000000000000000000}, // sqrt(25) = 5
    {100.00000000000000000000000000000000000, 10.000000000000000000000000000000000000}, // sqrt(100) = 10
};

struct golden_test_t {
    f64 (*f)(f64);
    func_test_t const *tests;
    usize test_count;
    char const *name;
    f64 allowed_error;
};

struct reference_test_t {
    f64 (*f)(f64);
    f64 (*reference)(f64);
    f64 test_range_min, test_range_max;
    u32 test_count;
    char const *name, *rname;
    f64 allowed_error;
};

#define GTEST(f_, ts_, aerr_) golden_test_t{&f_, ts_, ARR_CNT(ts_), #f_, aerr_}  
#define RTEST(f_, rf_, trmin_, trmax_, tcnt_, aerr_) \
    reference_test_t{&f_, &rf_, (trmin_), (trmax_), (tcnt_), #f_, #rf_, aerr_}  

#if VERBOSE
#define LOGVERBOSE(...) LOGNORMAL(__VA_ARGS__)
#else
#define LOGVERBOSE(...)
#endif

FINLINE f64 sqrt_s(f64 in)
{
    __m128d xmm = _mm_set_sd(in);
    __m128d sqrt_xmm = _mm_sqrt_sd(xmm, xmm);
    return _mm_cvtsd_f64(sqrt_xmm);
}

FINLINE f64 sqrt_dc(f64 in)
{
    __m128 xmm = _mm_set_ss(f32(in));
    __m128 sqrt_xmm = _mm_sqrt_ss(xmm);
    return f64(_mm_cvtss_f32(sqrt_xmm));
}

FINLINE f64 sqrt_approx(f64 in)
{
    __m128 xmm = _mm_set_ss(f32(in));
    __m128 rsqrt_xmm = _mm_rsqrt_ss(xmm);
    __m128 sqrt_xmm = _mm_rcp_ss(rsqrt_xmm);
    return f64(_mm_cvtss_f32(sqrt_xmm));
}

constexpr f64 c_pi_quat = 0.78539816339744830961566084581987572104929234984377645524373614807695410157155224965700870633552926699553702162832057666177346115238764555793133985203212027936257102567548463027638991115573723873259549;

// -3.0 / (c_pi * c_pi)
constexpr f64 c_sin_quad_a = -0.30396355092701331433163838962918291671307632401673964653682709568251936288670632357362782177686551284086673328455715874504218580295825523720801065449044977915856290581184826954827935928637383859959077;
// 7.0 / (2.0 * c_pi)
constexpr f64 c_sin_quad_b = 1.1140846016432673503821863436076005342412175201831951412336714084122775834395857456307966193637716016925098990873057062582503829228020505701457718751218988746046797685618635430115442247784771538210218;
constexpr f64 c_sin_seg_size = c_pi_half;

FINLINE f64 sin_range_reduction(f64 in, auto &&calc)
{
    __m128d xin = _mm_set_sd(in);
    __m128d segsz = _mm_set_sd(c_sin_seg_size);
    __m128d xwseg = _mm_div_sd(xin, segsz);
    __m128d xseg = _mm_floor_sd(xwseg, xwseg);
    u64 seg = u64(_mm_cvtsd_si64(xseg));
    f64 inseg = _mm_cvtsd_f64(_mm_sub_sd(xin, _mm_mul_sd(xseg, segsz)));
    f64 ysign = seg & 0b10 ? -1.0 : 1.0;
    f64 x = seg & 0b01 ? c_pi_half - inseg : inseg;
    return ysign * calc(x);
}

FINLINE f64 sin_quad_approx(f64 in)
{
    return sin_range_reduction(in,
        [](f64 x) { return c_sin_quad_a * x * x + c_sin_quad_b * x; });
}

FINLINE f64 cos_quad_approx(f64 in)
{
    return sin_quad_approx(in + c_pi_half);
}

template <usize t_n>
struct SinePolyCoeffs {
    f64 cs[t_n + 1];
};

enum sine_poly_coeff_technique_t {
    e_spct_taylor_pi_quat,
    e_spct_minimax_precomputed,
};

template <f64 t_point, usize t_n>
inline SinePolyCoeffs<t_n> calc_sine_taylor_coeffs()
{
    SinePolyCoeffs<t_n> res{};
    res.cs[0] = sin(t_point);

    auto derivative = [](usize n) {
        switch (n & 0b11) {
        case 0:
            return sin(t_point);
        case 1:
            return cos(t_point);
        case 2:
            return -sin(t_point);
        case 3:
            return -cos(t_point);
        default:
            return 0.0;
        }
    };

    u64 factorial_accum = 1;
    for (usize i = 1; i <= t_n; ++i) {
        factorial_accum *= i;
        res.cs[i] = derivative(i) / f64(factorial_accum);
    }

    return res;
}

inline constexpr f64 c_sine_minimax_coeff_arrays[][22] =
{
    {},
    {0.0},
    {0.0, 0x1.fc4eac57b4a27p-1},
    {0.0, 0x1.fc4eac57b4a27p-1, 0.0},
    {0.0, 0x1.fc4eac57b4a27p-1, 0.0, -0x1.2b704cf682899p-3},
    {0.0, 0x1.fff1d21fa9dedp-1, 0.0, -0x1.53e2e5c7dd831p-3, 0.0},
    {0.0, 0x1.fff1d21fa9dedp-1, 0.0, -0x1.53e2e5c7dd831p-3, 0.0, 0x1.f2438d36d9dbbp-8},
    {0.0, 0x1.ffffe07d31fe8p-1, 0.0, -0x1.554f800fc5ea1p-3, 0.0, 0x1.105d44e6222ap-7, 0.0},
    {0.0, 0x1.ffffe07d31fe8p-1, 0.0, -0x1.554f800fc5ea1p-3, 0.0, 0x1.105d44e6222ap-7, 0.0, -0x1.83b9725dff6e8p-13},
    {0.0, 0x1.ffffffd25a681p-1, 0.0, -0x1.555547ef5150bp-3, 0.0, 0x1.110e7b396c557p-7, 0.0, -0x1.9f6445023f795p-13, 0.0},
    {0.0, 0x1.ffffffd25a681p-1, 0.0, -0x1.555547ef5150bp-3, 0.0, 0x1.110e7b396c557p-7, 0.0, -0x1.9f6445023f795p-13, 0.0, 0x1.5d38b56aee7f1p-19},
    {0.0, 0x1.ffffffffd17d1p-1, 0.0, -0x1.55555541759fap-3, 0.0, 0x1.11110b74adb14p-7, 0.0, -0x1.a017a8fe15033p-13, 0.0, 0x1.716ba4fe56f6ep-19, 0.0},
    {0.0, 0x1.ffffffffd17d1p-1, 0.0, -0x1.55555541759fap-3, 0.0, 0x1.11110b74adb14p-7, 0.0, -0x1.a017a8fe15033p-13, 0.0, 0x1.716ba4fe56f6ep-19, 0.0, -0x1.9a0e192a4e2cbp-26},
    {0.0, 0x1.ffffffffffdcep-1, 0.0, -0x1.5555555540b9bp-3, 0.0, 0x1.111111090f0bcp-7, 0.0, -0x1.a019fce979937p-13, 0.0, 0x1.71dce5ace58d2p-19, 0.0, -0x1.ae00fd733fe8dp-26, 0.0},
    {0.0, 0x1.ffffffffffdcep-1, 0.0, -0x1.5555555540b9bp-3, 0.0, 0x1.111111090f0bcp-7, 0.0, -0x1.a019fce979937p-13, 0.0, 0x1.71dce5ace58d2p-19, 0.0, -0x1.ae00fd733fe8dp-26, 0.0, 0x1.52ace959bd023p-33},
    {0.0, 0x1.fffffffffffffp-1, 0.0, -0x1.5555555555469p-3, 0.0, 0x1.111111110941dp-7, 0.0, -0x1.a01a0199e0eb3p-13, 0.0, 0x1.71de37e62aacap-19, 0.0, -0x1.ae634d22bb47cp-26, 0.0, 0x1.60e59ae00e00cp-33, 0.0},
    {0.0, 0x1.fffffffffffffp-1, 0.0, -0x1.5555555555469p-3, 0.0, 0x1.111111110941dp-7, 0.0, -0x1.a01a0199e0eb3p-13, 0.0, 0x1.71de37e62aacap-19, 0.0, -0x1.ae634d22bb47cp-26, 0.0, 0x1.60e59ae00e00cp-33, 0.0, -0x1.9ef5d594b342p-41},
    {0.0, 0x1p0, 0.0, -0x1.5555555555555p-3, 0.0, 0x1.11111111110c9p-7, 0.0, -0x1.a01a01a014eb6p-13, 0.0, 0x1.71de3a52aab96p-19, 0.0, -0x1.ae6454d960ac4p-26, 0.0, 0x1.6123ce513b09fp-33, 0.0, -0x1.ae43dc9bf8ba7p-41, 0.0},
    {0.0, 0x1p0, 0.0, -0x1.5555555555555p-3, 0.0, 0x1.11111111110c9p-7, 0.0, -0x1.a01a01a014eb6p-13, 0.0, 0x1.71de3a52aab96p-19, 0.0, -0x1.ae6454d960ac4p-26, 0.0, 0x1.6123ce513b09fp-33, 0.0, -0x1.ae43dc9bf8ba7p-41, 0.0, 0x1.883c1c5deffbep-49},
    {0.0, 0x1p0, 0.0, -0x1.5555555555555p-3, 0.0, 0x1.11111111110dcp-7, 0.0, -0x1.a01a01a016ef6p-13, 0.0, 0x1.71de3a53fa85cp-19, 0.0, -0x1.ae6455b871494p-26, 0.0, 0x1.612421756f93fp-33, 0.0, -0x1.ae671378c3d43p-41, 0.0, 0x1.90277dafc8ab9p-49, 0.0},
    {0.0, 0x1p0, 0.0, -0x1.5555555555555p-3, 0.0, 0x1.11111111110dcp-7, 0.0, -0x1.a01a01a016ef6p-13, 0.0, 0x1.71de3a53fa85cp-19, 0.0, -0x1.ae6455b871494p-26, 0.0, 0x1.612421756f93fp-33, 0.0, -0x1.ae671378c3d43p-41, 0.0, 0x1.90277dafc8ab9p-49, 0.0, -0x1.78262e1f2709cp-58},
    {0.0, 0x1p0, 0.0, -0x1.5555555555555p-3, 0.0, 0x1.11111111110dp-7, 0.0, -0x1.a01a01a01559ap-13, 0.0, 0x1.71de3a52ad36dp-19, 0.0, -0x1.ae64549aa7ca9p-26, 0.0, 0x1.612392f66fdcdp-33, 0.0, -0x1.ae11556cad6c4p-41, 0.0, 0x1.71744c339ad03p-49, 0.0, 0x1.52947c90f8199p-55, 0.0},
    {0.0, 0x1p0, 0.0, -0x1.5555555555555p-3, 0.0, 0x1.11111111110dp-7, 0.0, -0x1.a01a01a01559ap-13, 0.0, 0x1.71de3a52ad36dp-19, 0.0, -0x1.ae64549aa7ca9p-26, 0.0, 0x1.612392f66fdcdp-33, 0.0, -0x1.ae11556cad6c4p-41, 0.0, 0x1.71744c339ad03p-49, 0.0, 0x1.52947c90f8199p-55, 0.0, -0x1.ff1898c107cfap-59},
};

template <usize t_n>
inline SinePolyCoeffs<t_n> calc_sine_minimax_coeffs()
{
    SinePolyCoeffs<t_n> res{};
    for (usize i = 0; f64 &out : res.cs)
        out = c_sine_minimax_coeff_arrays[t_n][i++];
    return res;
}

template <sine_poly_coeff_technique_t t_tech, usize t_n>
inline SinePolyCoeffs<t_n> const c_sine_poly_coeffs =
    t_tech == e_spct_taylor_pi_quat ?
        calc_sine_taylor_coeffs<c_pi_quat, t_n>() :
        calc_sine_minimax_coeffs<t_n>();

template <sine_poly_coeff_technique_t t_tech>
inline f64 calc_sine_base(f64 in)
{
    if constexpr (t_tech == e_spct_taylor_pi_quat)
        return in - c_pi_quat;
    else
        return in;
}

// For precision test only
template <usize t_n>
FINLINE f64 sin_taylor_approx_no_fmadd(f64 in)
{
    return sin_range_reduction(in,
        [](f64 x) {
            auto const &coeffs = c_sine_poly_coeffs<e_spct_taylor_pi_quat, t_n>;
            f64 res = coeffs.cs[t_n];
            f64 base = x - c_pi_quat;
            for (usize i = t_n; i > 0; --i) {
                res *= base;
                res += coeffs.cs[i - 1];
            }
            return res;
        });
}

template <usize t_n>
FINLINE f64 sin_taylor_approx_no_horner(f64 in)
{
    return sin_range_reduction(in,
        [](f64 x) {
            auto const &coeffs = c_sine_poly_coeffs<e_spct_taylor_pi_quat, t_n>;
            f64 res = coeffs.cs[0];
            f64 base = x - c_pi_quat;
            f64 next = base;
            for (usize i = 0; i < t_n; ++i) {
                res += next * coeffs.cs[i + 1];
                next *= base;
            }
            return res;
        });
}

template <usize t_n>
FINLINE f64 sin_taylor_approx_bw(f64 in)
{
    return sin_range_reduction(in,
        [](f64 x) {
            auto const &coeffs = c_sine_poly_coeffs<e_spct_taylor_pi_quat, t_n>;
            f64 res = 0.0;
            f64 base = x - c_pi_quat;
            for (isize i = t_n; i >= 0; --i) {
                f64 next = 1.0;
                for (usize j = 0; j < i; ++j)
                    next *= base;
                res += next * coeffs.cs[i];
            }
            return res;
        });
}

template <usize t_n>
FINLINE f64 cos_taylor_approx_no_fmadd(f64 in)
{
    return sin_taylor_approx_no_fmadd<t_n>(in + c_pi_half);
}

// Real stuff
template <sine_poly_coeff_technique_t t_tech, usize t_n>
FINLINE f64 sin_approx_templ(f64 in)
{
    return sin_range_reduction(in,
        [](f64 x) {
            auto const &coeffs = c_sine_poly_coeffs<t_tech, t_n>;
            __m128d res = _mm_set_sd(coeffs.cs[t_n]);
            __m128d base = _mm_set_sd(calc_sine_base<t_tech>(x));
            for (usize i = t_n; i > 0; --i)
                res = _mm_fmadd_sd(res, base, _mm_set_sd(coeffs.cs[i - 1]));
            return _mm_cvtsd_f64(res);
        });
}

template <sine_poly_coeff_technique_t t_tech, usize t_n>
FINLINE f64 cos_approx_templ(f64 in)
{
    return sin_approx_templ<t_tech, t_n>(in + c_pi_half);
}

template <usize t_n>
FINLINE f64 sin_taylor_approx(f64 in)
{
    return sin_approx_templ<e_spct_taylor_pi_quat, t_n>(in);
}
template <usize t_n>
FINLINE f64 cos_taylor_approx(f64 in)
{
    return cos_approx_templ<e_spct_taylor_pi_quat, t_n>(in);
}
template <usize t_n>
FINLINE f64 sin_minimax_approx(f64 in)
{
    return sin_approx_templ<e_spct_minimax_precomputed, t_n>(in);
}
template <usize t_n>
FINLINE f64 cos_minimax_approx(f64 in)
{
    return cos_approx_templ<e_spct_minimax_precomputed, t_n>(in);
}

int main(int argc, char **argv)
{
    init_os_process_state(g_os_proc_state);

    for (auto [f, ts, ts_cnt, nm, allowed_err] :
        {
            GTEST(cos, c_cosine_gold_standard, DBL_EPSILON),
            GTEST(asin, c_arcsine_gold_standard, DBL_EPSILON),
            GTEST(sqrt, c_sqrt_gold_standard, DBL_EPSILON)
        })
    {
        LOGNORMAL(
            "Golden test for %s. Allowed error=%.18lf", nm, allowed_err);
        for (usize i = 0; i < ts_cnt; ++i) {
            f64 const in = ts[i].in;
            f64 const res = (*f)(in);
            f64 const req = ts[i].out;
            f64 const err = abs(req - res);
            if (err > allowed_err) {
                LOGNORMAL(
                    "[FAIL] %s(%.18lf). "
                    "Required %.18lf, got %.18lf. Error=%.18lf",
                    nm, in, req, res, err);
            } else {
                LOGNORMAL(
                    "[OK]   %s(%.18lf) = %.18lf. Error=%.18lf",
                    nm, in, res, err);
            }
        }
    }

    LOGNORMAL("");

    for (auto [f, rf, tr_min, tr_max, tcnt, nm, rnm, allowed_err] :
        {
            //RTEST(asin, ..., 0.0, 1.0, 1024, 0.0),

            RTEST(sin_a, sin, -2.0 * c_pi, 2.0 * c_pi, 1024, f64(FLT_EPSILON)),
            RTEST(cos_a, cos, -2.0 * c_pi, 2.0 * c_pi, 1024, f64(FLT_EPSILON)),

            RTEST(sin_quad_approx, sin, -2.0 * c_pi, 2.0 * c_pi, 1024, f64(FLT_EPSILON)),
            RTEST(cos_quad_approx, cos, -2.0 * c_pi, 2.0 * c_pi, 1024, f64(FLT_EPSILON)),

            RTEST(sin_taylor_approx<2>, sin, -2.0 * c_pi, 2.0 * c_pi, 1024, f64(FLT_EPSILON)),
            RTEST(sin_minimax_approx<2>, sin, -2.0 * c_pi, 2.0 * c_pi, 1024, f64(FLT_EPSILON)),
            RTEST(sin_taylor_approx<5>, sin, -2.0 * c_pi, 2.0 * c_pi, 1024, f64(FLT_EPSILON)),
            RTEST(sin_minimax_approx<5>, sin, -2.0 * c_pi, 2.0 * c_pi, 1024, f64(FLT_EPSILON)),
            RTEST(sin_taylor_approx<8>, sin, -2.0 * c_pi, 2.0 * c_pi, 1024, f64(FLT_EPSILON)),
            RTEST(sin_minimax_approx<8>, sin, -2.0 * c_pi, 2.0 * c_pi, 1024, f64(FLT_EPSILON)),
            RTEST(sin_taylor_approx<9>, sin, -2.0 * c_pi, 2.0 * c_pi, 1024, f64(FLT_EPSILON)),
            RTEST(sin_minimax_approx<9>, sin, -2.0 * c_pi, 2.0 * c_pi, 1024, f64(FLT_EPSILON)),
            RTEST(sin_taylor_approx<10>, sin, -2.0 * c_pi, 2.0 * c_pi, 1024, f64(FLT_EPSILON)),
            RTEST(sin_minimax_approx<10>, sin, -2.0 * c_pi, 2.0 * c_pi, 1024, f64(FLT_EPSILON)),
            RTEST(sin_taylor_approx<11>, sin, -2.0 * c_pi, 2.0 * c_pi, 1024, f64(FLT_EPSILON)),
            RTEST(sin_minimax_approx<11>, sin, -2.0 * c_pi, 2.0 * c_pi, 1024, f64(FLT_EPSILON)),
            RTEST(sin_taylor_approx<16>, sin, -2.0 * c_pi, 2.0 * c_pi, 1024, f64(FLT_EPSILON)),
            RTEST(sin_minimax_approx<16>, sin, -2.0 * c_pi, 2.0 * c_pi, 1024, f64(FLT_EPSILON)),
            RTEST(sin_taylor_approx<17>, sin, -2.0 * c_pi, 2.0 * c_pi, 1024, f64(FLT_EPSILON)),
            RTEST(sin_minimax_approx<17>, sin, -2.0 * c_pi, 2.0 * c_pi, 1024, f64(FLT_EPSILON)),
            RTEST(sin_taylor_approx<19>, sin, -2.0 * c_pi, 2.0 * c_pi, 1024, f64(FLT_EPSILON)),
            RTEST(sin_minimax_approx<19>, sin, -2.0 * c_pi, 2.0 * c_pi, 1024, f64(FLT_EPSILON)),
            RTEST(sin_taylor_approx<21>, sin, -2.0 * c_pi, 2.0 * c_pi, 1024, f64(FLT_EPSILON)),
            RTEST(sin_minimax_approx<21>, sin, -2.0 * c_pi, 2.0 * c_pi, 1024, f64(FLT_EPSILON)),
            RTEST(sin_taylor_approx<22>, sin, -2.0 * c_pi, 2.0 * c_pi, 1024, f64(FLT_EPSILON)),
            RTEST(sin_minimax_approx<22>, sin, -2.0 * c_pi, 2.0 * c_pi, 1024, f64(FLT_EPSILON)),

            RTEST(cos_taylor_approx<2>, cos, -2.0 * c_pi, 2.0 * c_pi, 1024, f64(FLT_EPSILON)),
            RTEST(cos_minimax_approx<2>, cos, -2.0 * c_pi, 2.0 * c_pi, 1024, f64(FLT_EPSILON)),
            RTEST(cos_taylor_approx<5>, cos, -2.0 * c_pi, 2.0 * c_pi, 1024, f64(FLT_EPSILON)),
            RTEST(cos_minimax_approx<5>, cos, -2.0 * c_pi, 2.0 * c_pi, 1024, f64(FLT_EPSILON)),
            RTEST(cos_taylor_approx<8>, cos, -2.0 * c_pi, 2.0 * c_pi, 1024, f64(FLT_EPSILON)),
            RTEST(cos_minimax_approx<8>, cos, -2.0 * c_pi, 2.0 * c_pi, 1024, f64(FLT_EPSILON)),
            RTEST(cos_taylor_approx<9>, cos, -2.0 * c_pi, 2.0 * c_pi, 1024, f64(FLT_EPSILON)),
            RTEST(cos_minimax_approx<9>, cos, -2.0 * c_pi, 2.0 * c_pi, 1024, f64(FLT_EPSILON)),
            RTEST(cos_taylor_approx<10>, cos, -2.0 * c_pi, 2.0 * c_pi, 1024, f64(FLT_EPSILON)),
            RTEST(cos_minimax_approx<10>, cos, -2.0 * c_pi, 2.0 * c_pi, 1024, f64(FLT_EPSILON)),
            RTEST(cos_taylor_approx<11>, cos, -2.0 * c_pi, 2.0 * c_pi, 1024, f64(FLT_EPSILON)),
            RTEST(cos_minimax_approx<11>, cos, -2.0 * c_pi, 2.0 * c_pi, 1024, f64(FLT_EPSILON)),
            RTEST(cos_taylor_approx<16>, cos, -2.0 * c_pi, 2.0 * c_pi, 1024, f64(FLT_EPSILON)),
            RTEST(cos_minimax_approx<16>, cos, -2.0 * c_pi, 2.0 * c_pi, 1024, f64(FLT_EPSILON)),
            RTEST(cos_taylor_approx<17>, cos, -2.0 * c_pi, 2.0 * c_pi, 1024, f64(FLT_EPSILON)),
            RTEST(cos_minimax_approx<17>, cos, -2.0 * c_pi, 2.0 * c_pi, 1024, f64(FLT_EPSILON)),
            RTEST(cos_taylor_approx<19>, cos, -2.0 * c_pi, 2.0 * c_pi, 1024, f64(FLT_EPSILON)),
            RTEST(cos_minimax_approx<19>, cos, -2.0 * c_pi, 2.0 * c_pi, 1024, f64(FLT_EPSILON)),
            RTEST(cos_taylor_approx<21>, cos, -2.0 * c_pi, 2.0 * c_pi, 1024, f64(FLT_EPSILON)),
            RTEST(cos_minimax_approx<21>, cos, -2.0 * c_pi, 2.0 * c_pi, 1024, f64(FLT_EPSILON)),
            RTEST(cos_taylor_approx<22>, cos, -2.0 * c_pi, 2.0 * c_pi, 1024, f64(FLT_EPSILON)),
            RTEST(cos_minimax_approx<22>, cos, -2.0 * c_pi, 2.0 * c_pi, 1024, f64(FLT_EPSILON)),

            RTEST(sqrt_s, sqrt, 0.0, 1.0, 1024, f64(FLT_EPSILON)),
            RTEST(sqrt_dc, sqrt, 0.0, 1.0, 1024, f64(FLT_EPSILON)),
            RTEST(sqrt_approx, sqrt, 0.0, 1.0, 1024, f64(FLT_EPSILON))
        })
    {
        LOGVERBOSE(
            "Reference test for %s (against %s) in [%.18lf, %.18lf]. "
            "Allowed error=%.18lf", nm, rnm, tr_min, tr_max, allowed_err);
        int errcnt = 0;
        float max_error = 0.0;
        float max_error_arg = DBL_MAX;
        for (usize i = 0; i < tcnt; ++i) {
            f64 const in = tr_min + (tr_max - tr_min) * f64(i) / (tcnt - 1);
            f64 const res = (*f)(in);
            f64 const req = (*rf)(in);
            f64 const err = abs(req - res);
            if (err > max_error) {
                max_error = err;
                max_error_arg = in;
            }
            if (err > allowed_err) {
                LOGVERBOSE(
                    "[FAIL] %s(%.18lf). Reference %s=%.18lf, "
                    "got %.18lf. Error=%.18lf", nm, in, rnm, req, res, err);
                ++errcnt;
            } else {
                LOGVERBOSE(
                    "[OK]   %s(%.18lf). Reference %s=%.18lf, "
                    "got %.18lf. Error=%.18lf", nm, in, rnm, req, res, err);
            }
        }
        LOGNORMAL("%s (against %s) in [%.18lf, %.18lf]:", nm, rnm, tr_min, tr_max);
        LOGNORMAL("    max error %.18lf (at %.18lf)", max_error, max_error_arg);
        LOGNORMAL("    %d/%d under allowed error level of %.18lf", tcnt - errcnt, tcnt, allowed_err);
    }
}


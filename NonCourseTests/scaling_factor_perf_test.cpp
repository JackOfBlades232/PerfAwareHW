#include <os.hpp>
#include <profiling.hpp>
#include <logging.hpp>
#include <repetition.hpp>
#include <buffer.hpp>
#include <benchmark.hpp>
#include <intrinsics.hpp>
#define RT_STOP_TIME 10.f

namespace dag
{

template <class _To, class _From>
requires(sizeof(_To) == sizeof(_From))
[[nodiscard]] constexpr _To bit_cast(const _From &_Val) noexcept
{
    return __builtin_bit_cast(_To, _Val);
}

} // namespace dag

    template <typename To, typename From>
__forceinline To bitwise_cast(const From &from)
{
    return dag::bit_cast<To>(from);
}

// From https://gist.github.com/Marc-B-Reynolds/739a46f55c2a9ead54f4d0629ee5e417

static FINLINE f32 f32_cbrt_ki_(f32 x, u32 xi)
{
    // authors optimized Halley's method first step
    // given their "magic" constant (in 1)
    const f32 c0 = 0x1.c09806p0f;
    const f32 c1 = -0x1.403e6cp0f;
    const f32 c2 = 0x1.04cdb2p-1f;

    // initial guess
    xi = 0x548c2b4b - xi / 3; // (1)

    // modified Halley's method
    f32 y = bitwise_cast<f32>(xi);
    f32 c = (x * y) * (y * y); // (2)

    y = y * fmaf(c, fmaf(c2, c, c1), c0);

    return y;
}

static FINLINE f32 f32_cbrt_newton_fast_(f32 x, f32 y)
{
    const f32 f32_third = 0x1.555556p-2f;
    f32 a = x * y;
    f32 d = a * y;
    f32 c = fmaf(d, y, -1.f);
    f32 t = fmaf(c, -f32_third, 1.f);
    y = d * (t * t);

    return y;
}

static f32 cbrt_approx_nonneg(f32 x) // x must be positive
{
    f32 y = f32_cbrt_ki_(x, bitwise_cast<u32>(x));
    return f32_cbrt_newton_fast_(x, y);
}

struct Point3 {
    f32 x, y, z;
};

struct TMatrix {
    union {
        f32 m[4][3];
        f32 array[12];
        Point3 col[4];
    };

    f32 det() const {
        return
            m[0][0] * m[1][1] * m[2][2] + m[0][1] * m[1][2] * m[2][0] +
            m[1][0] * m[2][1] * m[0][2] - m[0][2] * m[1][1] * m[2][0] -
            m[0][1] * m[1][0] * m[2][2] - m[1][2] * m[2][1] * m[0][0];
    }
    f32 getScalingFactor() const { return cbrt_approx_nonneg(abs(det())); }
};

struct alignas(16) Point3_vec4 : public Point3 {
    f32 resv;
    Point3_vec4 &operator=(const Point3 &b)
    {
        Point3::operator=(b);
        resv = 0.f;
        return *this;
    }
};

typedef __m128 vec4f;
typedef __m128i vec4i;
typedef __m128 vec3f;
struct mat44f {
    vec4f col0, col1, col2, col3;
};

FINLINE vec4f v_xor(vec4f a, vec4f b) { return _mm_xor_ps(a,b); }
FINLINE vec4f v_cast_vec4f(vec4i a) {return _mm_castsi128_ps(a);}//no instruction
FINLINE vec4i v_splatsi(int a) {return _mm_set1_epi32(a);}
FINLINE vec4f v_neg(vec4f a) {return v_xor(a, v_cast_vec4f(v_splatsi(0x80000000)));}
FINLINE vec4f v_max(vec4f a, vec4f b) { return _mm_max_ps(a, b); }
#define _MM_SHUFFLE(z, y, x, w) (((z) << 6) | ((y) << 4) | ((x) << 2) | (w))
#define V_SHUFFLE(v, mask) _mm_shuffle_ps(v, v, mask)
#define V_SHUFFLE_REV(v, maskW, maskZ, maskY, maskX) V_SHUFFLE(v, _MM_SHUFFLE(maskW, maskZ, maskY, maskX))
FINLINE vec4f v_perm_yzxw(vec4f a) { return V_SHUFFLE_REV(a, 3,0,2,1); }
FINLINE vec4f v_perm_yzxy(vec4f a) { return V_SHUFFLE_REV(a, 1,0,2,1); }
FINLINE vec4f v_nmsub(vec4f a, vec4f b, vec4f c) { return _mm_sub_ps(c, _mm_mul_ps(a, b)); }
FINLINE vec4f v_mul(vec4f a, vec4f b) { return _mm_mul_ps(a, b); }
FINLINE vec3f v_cross3(vec3f a, vec3f b)
{
    // (a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x)
    vec3f yzxw = v_perm_yzxw(a);
    vec3f bcad = v_perm_yzxw(b);
    return v_perm_yzxy(v_nmsub(yzxw, b, v_mul(a, bcad)));
}
FINLINE vec4f sse4_dot3(vec4f a, vec4f b) { return _mm_dp_ps(a,b, 0x7F); }
FINLINE vec4f v_dot3(vec4f a, vec4f b) { return sse4_dot3(a,b); }
FINLINE vec4f v_mat44_det43(const mat44f &m) { return v_dot3(m.col2, v_cross3(m.col0, m.col1)); }
FINLINE vec4f v_mat44_det43_cols(vec3f c0, vec3f c1, vec3f c2) { return v_dot3(c2, v_cross3(c0, c1)); }
FINLINE vec4f v_abs(vec4f a) { return v_max(v_neg(a), a); }
FINLINE f32 v_extract_x(vec4f a) { return _mm_cvtss_f32(a); }
FINLINE vec4f v_ldu(const f32 *m) { return _mm_loadu_ps(m); }

static f32 get_tm_scaling_factor(const TMatrix &tm)
{
    return tm.getScalingFactor();
}

static f32 get_tm_scaling_factor_fast(const TMatrix &tm)
{
    vec3f c0 = v_ldu((const f32 *)&tm.col[0]);
    vec3f c1 = v_ldu((const f32 *)&tm.col[1]);
    vec3f c2 = v_ldu((const f32 *)&tm.col[2]);
    return cbrt_approx_nonneg(fabsf(v_extract_x(v_mat44_det43_cols(c0, c1, c2))));
}

struct tested_func_t {
    f32 (*f)(const TMatrix &);
    char const *name;
};

#define TEST_FUNC(f_) tested_func_t{&f_, #f_}

struct verified_func_t {
    tested_func_t subject;
    tested_func_t example;
};

int main(int argc, char **argv)
{
    if (argc < 2)
        return 1;

    usize const mat_count = atol(argv[1]);
    if (mat_count < 1)
        return 1;

    init_os_process_state(g_os_proc_state);
    u64 cpu_timer_freq = measure_cpu_timer_freq(0.1l);

    usize const byte_count = mat_count * sizeof(TMatrix);
    buffer_t input = allocate_best(byte_count);
    if (!is_valid(input))
        return 2;

    srand(time(nullptr));

    TMatrix *mat_input = (TMatrix *)input.data;
    f32 *f32_input = (f32 *)input.data;

    for (usize i = 0; i < mat_count * sizeof(TMatrix) / sizeof(f32); ++i)
        f32_input[i] = 100.f * (f32(rand()) / RAND_MAX);

    constexpr verified_func_t c_ver_funcs[] =
    {
        {
            TEST_FUNC(get_tm_scaling_factor_fast),
            TEST_FUNC(get_tm_scaling_factor)
        },
    };

    for (auto [s, e] : c_ver_funcs) {
        f64 avg_error = 0.0;
        f64 max_error = 0.0;
        f64 avg_rel_error = 0.0;
        f64 max_rel_error = 0.0;
        
        for (usize i = 0; i < mat_count; ++i) {
            f64 sr = s.f(mat_input[i]);
            f64 er = e.f(mat_input[i]);
            f64 error = abs(sr - er);
            f64 rel_error = er > 0.0 ? error / er : error;
            avg_error += error;
            avg_rel_error += rel_error;
            max_error = max(max_error, error);
            max_rel_error = max(max_rel_error, rel_error);
        }
        avg_error /= f64(mat_count);
        avg_rel_error /= f64(mat_count);

        printf(
            "%s (vs %s): AErr=%lf, MErr=%lf, ARErr=%lf, MRErr=%lf\n",
            s.name, e.name, avg_error, max_error, avg_rel_error, max_rel_error);
    }

    constexpr tested_func_t c_test_funcs[] =
    {
        TEST_FUNC(get_tm_scaling_factor),
        TEST_FUNC(get_tm_scaling_factor_fast),
    };

    repetition_test_results_t results{};
    RepetitionTester rt{
        mat_count * sizeof(TMatrix), cpu_timer_freq, RT_STOP_TIME, true};
    for (auto [f, name] : c_test_funcs) {
        rt.ReStart(results, mat_count * sizeof(TMatrix));
        do {
            TMatrix *p = mat_input;

            rt.BeginTimeBlock();
            for (usize i = 0; i < mat_count; ++i)
                BENCHMARK_CONSUME(f(*p++));
            rt.EndTimeBlock();

            rt.ReportProcessedBytes((u8 *)p - (u8 *)mat_input);
        } while (rt.Tick());

        {
            char namebuf[256];
            snprintf(
                namebuf, sizeof(namebuf), "%s (%llu matrices)",
                name, mat_count);
            print_reptest_results(
                results, byte_count, cpu_timer_freq, namebuf, true);
        }
    }
}

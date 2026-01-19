#include <os.hpp>
#include <profiling.hpp>
#include <logging.hpp>
#include <repetition.hpp>
#include <buffer.hpp>
#include <benchmark.hpp>
#define RT_STOP_TIME 10.f

struct tested_func_t {
    f32 (*f)(f32);
    char const *name;
};

#define TEST_FUNC(f_) tested_func_t{&f_, #f_}

struct verified_func_t {
    tested_func_t subject;
    tested_func_t example;
};

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
__forceinline To bitwise_cast(const From &from) // To consider: may be just make it alias to `dag::bit_cast`?
{
  return dag::bit_cast<To>(from);
}


// From https://gist.github.com/Marc-B-Reynolds/739a46f55c2a9ead54f4d0629ee5e417

static FINLINE f32 f32_cbrt_ki_(f32 x, uint32_t xi)
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
    f32 y = f32_cbrt_ki_(x, bitwise_cast<uint32_t>(x));
    return f32_cbrt_newton_fast_(x, y);
}

static f32 cbrt_approx(f32 x)
{
    constexpr uint32_t SIGNMASK = (1u << 31);
    uint32_t xi = bitwise_cast<uint32_t>(x);
    uint32_t s = SIGNMASK & xi;
    uint32_t axi = xi & ~SIGNMASK;
    f32 ax = bitwise_cast<f32>(axi);
    f32 acbrt = cbrt_approx_nonneg(ax);
    return bitwise_cast<f32>(bitwise_cast<uint32_t>(acbrt) | s);
}


int main(int argc, char **argv)
{
    if (argc < 2)
        return 1;

    usize const f32_count = atol(argv[1]);
    if (f32_count < 2)
        return 1;

    init_os_process_state(g_os_proc_state);
    u64 cpu_timer_freq = measure_cpu_timer_freq(0.1l);

    usize const byte_count = f32_count * sizeof(f32);
    buffer_t input = allocate_best(byte_count);
    if (!is_valid(input))
        return 2;

    srand(time(nullptr));

    usize const uns_count = f32_count / 2;
    usize const sn_count = f32_count - uns_count;
    f32 *uns_input = (f32 *)input.data;
    f32 *sn_input = ((f32 *)input.data) + uns_count;

    for (usize i = 0; i < uns_count; ++i)
        uns_input[i] = 100.f * (f32(rand()) / RAND_MAX);
    for (usize i = 0; i < sn_count; ++i)
        sn_input[i] = 200.f * (f32(rand()) / RAND_MAX) - 100.f;

    constexpr verified_func_t c_ver_funcs_uns[] =
    {
        {
            TEST_FUNC(cbrt_approx_nonneg),
            TEST_FUNC(cbrt)
        },
    };

    constexpr verified_func_t c_ver_funcs_sn[] =
    {
        {
            TEST_FUNC(cbrt_approx),
            TEST_FUNC(cbrt)
        }
    };

    for (auto [s, e] : c_ver_funcs_uns) {
        f64 avg_error = 0.0;
        f64 max_error = 0.0;
        f64 avg_rel_error = 0.0;
        f64 max_rel_error = 0.0;
        
        for (usize i = 0; i < uns_count; ++i) {
            f64 sr = s.f(uns_input[i]);
            f64 er = e.f(uns_input[i]);
            f64 error = abs(sr - er);
            f64 rel_error = er > 0.0 ? error / er : error;
            avg_error += error;
            avg_rel_error += rel_error;
            max_error = max(max_error, error);
            max_rel_error = max(max_rel_error, rel_error);
        }
        avg_error /= f64(uns_count);
        avg_rel_error /= f64(uns_count);

        printf(
            "%s (vs %s, nonneg): AErr=%lf, MErr=%lf, ARErr=%lf, MRErr=%lf\n",
            s.name, e.name, avg_error, max_error, avg_rel_error, max_rel_error);
    }

    for (auto [s, e] : c_ver_funcs_sn) {
        f64 avg_error = 0.0;
        f64 max_error = 0.0;
        f64 avg_rel_error = 0.0;
        f64 max_rel_error = 0.0;
        
        for (usize i = 0; i < sn_count; ++i) {
            f64 sr = s.f(sn_input[i]);
            f64 er = e.f(sn_input[i]);
            f64 error = abs(sr - er);
            f64 rel_error = er > 0.0 ? error / er : error;
            avg_error += error;
            avg_rel_error += rel_error;
            max_error = max(max_error, error);
            max_rel_error = max(max_rel_error, rel_error);
        }
        avg_error /= f64(sn_count);
        avg_rel_error /= f64(sn_count);

        printf(
            "%s (vs %s): AErr=%lf, MErr=%lf, ARErr=%lf, MRErr=%lf\n",
            s.name, e.name, avg_error, max_error, avg_rel_error, max_rel_error);
    }

    constexpr tested_func_t c_test_funcs_uns[] =
    {
        TEST_FUNC(cbrt),
        TEST_FUNC(cbrt_approx_nonneg),
    };

    constexpr tested_func_t c_test_funcs_sn[] =
    {
        TEST_FUNC(cbrt),
        TEST_FUNC(cbrt_approx),
    };

    repetition_test_results_t results{};

    {
        RepetitionTester rt{
            uns_count * sizeof(f32), cpu_timer_freq, RT_STOP_TIME, true};
        for (auto [f, name] : c_test_funcs_uns) {
            rt.ReStart(results, uns_count * sizeof(f32));
            do {
                f32 *p = uns_input;

                rt.BeginTimeBlock();
                for (usize i = 0; i < uns_count; ++i)
                    BENCHMARK_CONSUME(f(*p++));
                rt.EndTimeBlock();

                rt.ReportProcessedBytes((u8 *)p - (u8 *)uns_input);
            } while (rt.Tick());

            {
                char namebuf[256];
                snprintf(
                    namebuf, sizeof(namebuf), "%s (%llu nonnegative f32s)",
                    name, uns_count);
                print_reptest_results(
                    results, byte_count, cpu_timer_freq, namebuf, true);
            }
        }
    }

    {
        RepetitionTester rt{
            sn_count * sizeof(f32), cpu_timer_freq, RT_STOP_TIME, true};
        for (auto [f, name] : c_test_funcs_sn) {
            rt.ReStart(results, sn_count * sizeof(f32));
            do {
                f32 *p = sn_input;

                rt.BeginTimeBlock();
                for (usize i = 0; i < sn_count; ++i)
                    BENCHMARK_CONSUME(f(*p++));
                rt.EndTimeBlock();

                rt.ReportProcessedBytes((u8 *)p - (u8 *)sn_input);
            } while (rt.Tick());

            {
                char namebuf[256];
                snprintf(
                    namebuf, sizeof(namebuf), "%s (%llu f32s)",
                    name, sn_count);
                print_reptest_results(
                    results, byte_count, cpu_timer_freq, namebuf, true);
            }
        }
    }
}

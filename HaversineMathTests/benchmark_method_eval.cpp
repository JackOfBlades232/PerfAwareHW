#include <haversine_math.hpp>

#include <os.hpp>
#include <profiling.hpp>
#include <logging.hpp>
#include <repetition.hpp>

#define RT_STOP_TIME 10.f

struct tested_calc_func_t {
    void (*f)(usize);
    char const *name;
};

#define TEST_FUNC(f_) tested_calc_func_t{&f_, #f_}

static FINLINE f64 asin_a_core(f64 x)
{
    f64 x2 = x * x;

    f64 r = 0x1.7f820d52c2775p-1;
    r = fmadd(r, x2, -0x1.4d84801ff1aa1p1);
    r = fmadd(r, x2, 0x1.14672d35db97ep2);
    r = fmadd(r, x2, -0x1.188f223fe5f34p2);
    r = fmadd(r, x2, 0x1.86bbff2a6c7b6p1);
    r = fmadd(r, x2, -0x1.83633c76e4551p0);
    r = fmadd(r, x2, 0x1.224c4dbe13cbdp-1);
    r = fmadd(r, x2, -0x1.2ab04ba9012e3p-3);
    r = fmadd(r, x2, 0x1.5565a3d3908b9p-5);
    r = fmadd(r, x2, 0x1.b1b8d27cd7e72p-8);
    r = fmadd(r, x2, 0x1.dc086c5d99cdcp-7);
    r = fmadd(r, x2, 0x1.1b8cc838ee86ep-6);
    r = fmadd(r, x2, 0x1.6e96be6dbe49ep-6);
    r = fmadd(r, x2, 0x1.f1c6b0ea300d7p-6);
    r = fmadd(r, x2, 0x1.6db6dca9f82d4p-5);
    r = fmadd(r, x2, 0x1.3333333148aa7p-4);
    r = fmadd(r, x2, 0x1.555555555683fp-3);
    r = fmadd(r, x2, 0x1.fffffffffffffp-1);
    r *= x;

    return r;
}

static FINLINE void escape(void *p)
{
#if __clang__ || __GNUC__
    asm volatile("" :: "g"(p) : "memory");
#else
    (void)p;
#endif
}

static FINLINE void writesim(auto &v)
{
#if __clang__ || __GNUC__
    asm volatile("" : "=x"(v));
#else
    (void)p;
#endif
}

static FINLINE void readsim(auto const &v)
{
#if __clang__ || __GNUC__
    asm volatile("" :: "x"(v));
#else
    (void)p;
#endif
}

static void bench_escape(usize rep_count)
{
    for (usize i = 0; i < rep_count; ++i) {
        f64 value = 0.5;
        escape(&value);
        f64 result = asin_a_core(value);
        escape(&result);
    }
}

// @NOTE: this actually neither works nor is correct:
// the compiler assumes that writesim actually writes the value into the
// register, discarding 0.5, and then assumes it can reuse the register
// for the result, thus leading to
// 1) Wrong answers
// 2) Loop carried dependency killing pipelining and perf
static void bench_rwsim(usize rep_count)
{
    for (usize i = 0; i < rep_count; ++i) {
        f64 value = 0.5;
        writesim(value);
        f64 result = asin_a_core(value);
        readsim(result);
    }
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        LOGERR("Usage: <program> [rep count]");
        return 1;
    }

    usize const rep_count = atol(argv[1]);

    init_os_process_state(g_os_proc_state);
    u64 cpu_timer_freq = measure_cpu_timer_freq(0.1l);

    constexpr tested_calc_func_t c_test_funcs[] =
    {
        TEST_FUNC(bench_escape),
        TEST_FUNC(bench_rwsim),
    };

    RepetitionTester rt{rep_count, cpu_timer_freq, RT_STOP_TIME, true};
    repetition_test_results_t results{};

    for (auto [f, name] : c_test_funcs) {
        rt.ReStart(results, rep_count);
        do {
            rt.BeginTimeBlock();
            (*f)(rep_count);
            rt.EndTimeBlock();

            rt.ReportProcessedBytes(rep_count);
        } while (rt.Tick());

        print_reptest_results(results, rep_count, cpu_timer_freq, name, true);
    }
}

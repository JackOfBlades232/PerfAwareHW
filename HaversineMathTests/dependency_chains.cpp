#include <haversine_math.hpp>

#include <os.hpp>
#include <profiling.hpp>
#include <logging.hpp>
#include <repetition.hpp>
#include <benchmark.hpp>

#ifndef RT_STOP_TIME
#define RT_STOP_TIME 10.0f
#endif

extern "C"
{
extern void fma_depchain_asm(u64 chain_count, u64 chain_len);
extern void fma_depchain_interleaved2_asm(u64 chain_count, u64 chain_len);
extern void fma_depchain_interleaved4_asm(u64 chain_count, u64 chain_len);
extern void fma_depchain_interleaved8_asm(u64 chain_count, u64 chain_len);
extern void fma_depchain_interleaved8x2_asm(u64 chain_count, u64 chain_len);
}

static void fma_depchain(u64 chain_count, u64 chain_len)
{
    for (u64 chain_id = 0; chain_id < chain_count; ++chain_id) {
        f64 x2 = 0.0;
        f64 m = 0.0;
        f64 r0 = 0.0;

        BENCHMARK_CLOBBER_F64(x2);
        BENCHMARK_CLOBBER_F64(m);
        BENCHMARK_CLOBBER_F64(r0);

        for (u64 iter = 0; iter < chain_len; iter += 8) {
            r0 = fmadd(r0, x2, m);
            r0 = fmadd(r0, x2, m);
            r0 = fmadd(r0, x2, m);
            r0 = fmadd(r0, x2, m);
            r0 = fmadd(r0, x2, m);
            r0 = fmadd(r0, x2, m);
            r0 = fmadd(r0, x2, m);
            r0 = fmadd(r0, x2, m);
        }

        BENCHMARK_CONSUME(r0);
    }
}

static void fma_depchain_interleaved2(u64 chain_count, u64 chain_len)
{
    for (u64 chain_id = 0; chain_id < chain_count; chain_id += 2) {
        f64 x2 = 0.0;
        f64 m = 0.0;
        f64 r0 = 0.0;
        f64 r1 = 0.0;

        BENCHMARK_CLOBBER_F64(x2);
        BENCHMARK_CLOBBER_F64(m);
        BENCHMARK_CLOBBER_F64(r0);
        BENCHMARK_CLOBBER_F64(r1);

        for (u64 iter = 0; iter < chain_len; iter += 4) {
            r0 = fmadd(r0, x2, m);
            r1 = fmadd(r1, x2, m);
            r0 = fmadd(r0, x2, m);
            r1 = fmadd(r1, x2, m);
            r0 = fmadd(r0, x2, m);
            r1 = fmadd(r1, x2, m);
            r0 = fmadd(r0, x2, m);
            r1 = fmadd(r1, x2, m);
        }

        BENCHMARK_CONSUME(r0);
        BENCHMARK_CONSUME(r1);
    }
}

static void fma_depchain_interleaved4(u64 chain_count, u64 chain_len)
{
    for (u64 chain_id = 0; chain_id < chain_count; chain_id += 4) {
        f64 x2 = 0.0;
        f64 m = 0.0;
        f64 r0 = 0.0;
        f64 r1 = 0.0;
        f64 r2 = 0.0;
        f64 r3 = 0.0;

        BENCHMARK_CLOBBER_F64(x2);
        BENCHMARK_CLOBBER_F64(m);
        BENCHMARK_CLOBBER_F64(r0);
        BENCHMARK_CLOBBER_F64(r1);
        BENCHMARK_CLOBBER_F64(r2);
        BENCHMARK_CLOBBER_F64(r3);

        for (u64 iter = 0; iter < chain_len; iter += 2) {
            r0 = fmadd(r0, x2, m);
            r1 = fmadd(r1, x2, m);
            r2 = fmadd(r2, x2, m);
            r3 = fmadd(r3, x2, m);
            r0 = fmadd(r0, x2, m);
            r1 = fmadd(r1, x2, m);
            r2 = fmadd(r2, x2, m);
            r3 = fmadd(r3, x2, m);
        }

        BENCHMARK_CONSUME(r0);
        BENCHMARK_CONSUME(r1);
        BENCHMARK_CONSUME(r2);
        BENCHMARK_CONSUME(r3);
    }
}

static void fma_depchain_interleaved8(u64 chain_count, u64 chain_len)
{
    for (u64 chain_id = 0; chain_id < chain_count; chain_id += 8) {
        f64 x2 = 0.0;
        f64 m = 0.0;
        f64 r0 = 0.0;
        f64 r1 = 0.0;
        f64 r2 = 0.0;
        f64 r3 = 0.0;
        f64 r4 = 0.0;
        f64 r5 = 0.0;
        f64 r6 = 0.0;
        f64 r7 = 0.0;

        BENCHMARK_CLOBBER_F64(x2);
        BENCHMARK_CLOBBER_F64(m);
        BENCHMARK_CLOBBER_F64(r0);
        BENCHMARK_CLOBBER_F64(r1);
        BENCHMARK_CLOBBER_F64(r2);
        BENCHMARK_CLOBBER_F64(r3);
        BENCHMARK_CLOBBER_F64(r4);
        BENCHMARK_CLOBBER_F64(r5);
        BENCHMARK_CLOBBER_F64(r6);
        BENCHMARK_CLOBBER_F64(r7);

        for (u64 iter = 0; iter < chain_len; iter += 1) {
            r0 = fmadd(r0, x2, m);
            r1 = fmadd(r1, x2, m);
            r2 = fmadd(r2, x2, m);
            r3 = fmadd(r3, x2, m);
            r4 = fmadd(r4, x2, m);
            r5 = fmadd(r5, x2, m);
            r6 = fmadd(r6, x2, m);
            r7 = fmadd(r7, x2, m);
        }

        BENCHMARK_CONSUME(r0);
        BENCHMARK_CONSUME(r1);
        BENCHMARK_CONSUME(r2);
        BENCHMARK_CONSUME(r3);
        BENCHMARK_CONSUME(r4);
        BENCHMARK_CONSUME(r5);
        BENCHMARK_CONSUME(r6);
        BENCHMARK_CONSUME(r7);
    }
}

static void fma_depchain_interleaved8x2(u64 chain_count, u64 chain_len)
{
    for (u64 chain_id = 0; chain_id < chain_count; chain_id += 8) {
        f64 x2 = 0.0;
        f64 m = 0.0;
        f64 r0 = 0.0;
        f64 r1 = 0.0;
        f64 r2 = 0.0;
        f64 r3 = 0.0;
        f64 r4 = 0.0;
        f64 r5 = 0.0;
        f64 r6 = 0.0;
        f64 r7 = 0.0;

        BENCHMARK_CLOBBER_F64(x2);
        BENCHMARK_CLOBBER_F64(m);
        BENCHMARK_CLOBBER_F64(r0);
        BENCHMARK_CLOBBER_F64(r1);
        BENCHMARK_CLOBBER_F64(r2);
        BENCHMARK_CLOBBER_F64(r3);
        BENCHMARK_CLOBBER_F64(r4);
        BENCHMARK_CLOBBER_F64(r5);
        BENCHMARK_CLOBBER_F64(r6);
        BENCHMARK_CLOBBER_F64(r7);

        for (u64 iter = 0; iter < chain_len; iter += 2) {
            r0 = fmadd(r0, x2, m);
            r1 = fmadd(r1, x2, m);
            r2 = fmadd(r2, x2, m);
            r3 = fmadd(r3, x2, m);
            r4 = fmadd(r4, x2, m);
            r5 = fmadd(r5, x2, m);
            r6 = fmadd(r6, x2, m);
            r7 = fmadd(r7, x2, m);
            r0 = fmadd(r0, x2, m);
            r1 = fmadd(r1, x2, m);
            r2 = fmadd(r2, x2, m);
            r3 = fmadd(r3, x2, m);
            r4 = fmadd(r4, x2, m);
            r5 = fmadd(r5, x2, m);
            r6 = fmadd(r6, x2, m);
            r7 = fmadd(r7, x2, m);
        }

        BENCHMARK_CONSUME(r0);
        BENCHMARK_CONSUME(r1);
        BENCHMARK_CONSUME(r2);
        BENCHMARK_CONSUME(r3);
        BENCHMARK_CONSUME(r4);
        BENCHMARK_CONSUME(r5);
        BENCHMARK_CONSUME(r6);
        BENCHMARK_CONSUME(r7);
    }
}

static void run_test(
    RepetitionTester &rt, repetition_test_results_t &results,
    u32 rep_count, u64 cpu_timer_freq, char const *name, auto &&f)
{
    printf("%s", name);
    for (u64 chain_len = 8; chain_len <= 256; chain_len += 8) {
        char namebuf[128];
        snprintf(namebuf, sizeof(namebuf), "%u %s", chain_len, name);

        u64 chain_count = rep_count / chain_len;
        u64 real_rep_count = chain_count * chain_len;

        rt.ReStart(results, real_rep_count);
        do {
            rt.BeginTimeBlock();
            f(chain_count, chain_len);
            rt.EndTimeBlock();

            rt.ReportProcessedBytes(real_rep_count);
        } while (rt.Tick());

        print_reptest_results(results, real_rep_count, cpu_timer_freq, namebuf, true);
        printf(",%lf", best_bptick(results, real_rep_count));
    }
    printf("\n");
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        LOGERR("Usage: <program> [rep count]");
        return 1;
    }

    u64 const rep_count = atol(argv[1]);

    init_os_process_state(g_os_proc_state);
    u64 cpu_timer_freq = measure_cpu_timer_freq(0.1l);

    RepetitionTester rt{rep_count, cpu_timer_freq, RT_STOP_TIME, true};
    repetition_test_results_t results{};

    for (u64 chain_len = 8; chain_len <= 256; chain_len += 8)
        printf(",%u", chain_len);
    printf("\n");

    run_test(rt, results, rep_count, cpu_timer_freq, "linear asm", &fma_depchain_asm);
    run_test(rt, results, rep_count, cpu_timer_freq, "linear cpp", &fma_depchain);
    run_test(rt, results, rep_count, cpu_timer_freq, "2-interleaved asm", &fma_depchain_interleaved2_asm);
    run_test(rt, results, rep_count, cpu_timer_freq, "2-interleaved cpp", &fma_depchain_interleaved2);
    run_test(rt, results, rep_count, cpu_timer_freq, "4-interleaved asm", &fma_depchain_interleaved4_asm);
    run_test(rt, results, rep_count, cpu_timer_freq, "4-interleaved cpp", &fma_depchain_interleaved4);
    run_test(rt, results, rep_count, cpu_timer_freq, "8-interleaved asm", &fma_depchain_interleaved8_asm);
    run_test(rt, results, rep_count, cpu_timer_freq, "8-interleaved cpp", &fma_depchain_interleaved8);
    run_test(rt, results, rep_count, cpu_timer_freq, "8x2-interleaved asm", &fma_depchain_interleaved8x2_asm);
    run_test(rt, results, rep_count, cpu_timer_freq, "8x2-interleaved cpp", &fma_depchain_interleaved8x2);
}

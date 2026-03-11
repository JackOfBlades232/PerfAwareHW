#include <haversine_math.hpp>

#include <os.hpp>
#include <profiling.hpp>
#include <logging.hpp>
#include <repetition.hpp>
#include <benchmark.hpp>

#define RT_STOP_TIME 10.0f

static void fma_dependency_chain(u32 chain_count, u32 chain_len)
{
    for (u32 chain_id = 0; chain_id < chain_count; ++chain_id) {
        f64 x2 = 0.0;
        f64 m = 0.0;
        f64 r0 = 0.0;

        BENCHMARK_CLOBBER_F64(x2);
        BENCHMARK_CLOBBER_F64(m);
        BENCHMARK_CLOBBER_F64(r0);

        for (u32 iter = 0; iter < chain_len; iter += 8) {
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

template <u32 t_batch_size>
static void fma_dependency_chain_batched(u32 chain_count, u32 chain_len)
{
    for (u32 chain_id = 0; chain_id < chain_count; chain_id += t_batch_size) {
        struct {
            f64 x2 = 0.0;
            f64 m = 0.0;
            f64 r0 = 0.0;
        } state[t_batch_size] = {};

        for (auto &entry : state) {
            BENCHMARK_CLOBBER_F64(entry.x2);
            BENCHMARK_CLOBBER_F64(entry.m);
            BENCHMARK_CLOBBER_F64(entry.r0);
        }

        for (u32 iter = 0; iter < chain_len; iter += 8) {
            for (auto &entry : state) {
                entry.r0 = fmadd(entry.r0, entry.x2, entry.m);
                entry.r0 = fmadd(entry.r0, entry.x2, entry.m);
                entry.r0 = fmadd(entry.r0, entry.x2, entry.m);
                entry.r0 = fmadd(entry.r0, entry.x2, entry.m);
                entry.r0 = fmadd(entry.r0, entry.x2, entry.m);
                entry.r0 = fmadd(entry.r0, entry.x2, entry.m);
                entry.r0 = fmadd(entry.r0, entry.x2, entry.m);
                entry.r0 = fmadd(entry.r0, entry.x2, entry.m);
            }
        }

        for (auto &entry : state)
            BENCHMARK_CONSUME(entry.r0);
    }
}

static void run_test(
    RepetitionTester &rt, repetition_test_results_t &results,
    u32 rep_count, u64 cpu_timer_freq,
    auto &&f, auto &&namer)
{
    for (u32 chain_len = 8; chain_len <= 256; chain_len += 8) {
        char name[128];
        namer(name, chain_len);

        u32 chain_count = rep_count / chain_len;
        u32 real_rep_count = chain_count * chain_len;

        rt.ReStart(results, real_rep_count);
        do {
            rt.BeginTimeBlock();
            f(chain_count, chain_len);
            rt.EndTimeBlock();

            rt.ReportProcessedBytes(real_rep_count);
        } while (rt.Tick());

        print_reptest_results(results, real_rep_count, cpu_timer_freq, name, true);
    }
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        LOGERR("Usage: <program> [rep count]");
        return 1;
    }

    u32 const rep_count = atol(argv[1]);

    init_os_process_state(g_os_proc_state);
    u64 cpu_timer_freq = measure_cpu_timer_freq(0.1l);

    RepetitionTester rt{rep_count, cpu_timer_freq, RT_STOP_TIME, true};
    repetition_test_results_t results{};

    run_test(
        rt, results, rep_count, cpu_timer_freq,
        [&](u32 chain_count, u32 chain_len) { fma_dependency_chain(chain_count, chain_len); },
        [&](char (&buf)[128], u32 chain_len) { snprintf(buf, sizeof(buf), "%u linear", chain_len); });
    run_test(
        rt, results, rep_count, cpu_timer_freq,
        [&](u32 chain_count, u32 chain_len) { fma_dependency_chain_batched<2>(chain_count, chain_len); },
        [&](char (&buf)[128], u32 chain_len) { snprintf(buf, sizeof(buf), "%u 2-batch", chain_len); });
    run_test(
        rt, results, rep_count, cpu_timer_freq,
        [&](u32 chain_count, u32 chain_len) { fma_dependency_chain_batched<4>(chain_count, chain_len); },
        [&](char (&buf)[128], u32 chain_len) { snprintf(buf, sizeof(buf), "%u 4-batch", chain_len); });
    run_test(
        rt, results, rep_count, cpu_timer_freq,
        [&](u32 chain_count, u32 chain_len) { fma_dependency_chain_batched<8>(chain_count, chain_len); },
        [&](char (&buf)[128], u32 chain_len) { snprintf(buf, sizeof(buf), "%u 8-batch", chain_len); });
    run_test(
        rt, results, rep_count, cpu_timer_freq,
        [&](u32 chain_count, u32 chain_len) { fma_dependency_chain_batched<16>(chain_count, chain_len); },
        [&](char (&buf)[128], u32 chain_len) { snprintf(buf, sizeof(buf), "%u 16-batch", chain_len); });
}

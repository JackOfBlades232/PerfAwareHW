#include <repetition.hpp>
#include <profiling.hpp>
#include <memory.hpp>
#include <os.hpp>
#include <util.hpp>
#include <algo.hpp>

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstddef>
#include <cassert>

extern "C"
{
extern uint64_t run_loop_load_npot(uint64_t count, char *ptr, uint64_t sz);
}

static void fill_first_pass_sizes(uint64_t *sizes, int min_pot, int max_pot)
{
    uint64_t *p = sizes;
    for (int i = min_pot; i <= max_pot; ++i)
        *sizes++ = 1 << i;
}

static long double probe_cache_size(
    uint64_t iter_byte_cnt, char *mem,
    uint64_t probed_size, uint64_t cpu_timer_freq)
{
    static long double res = 0.0; // For having lambda be an fptr

    uint64_t const real_byte_cnt = round_up(iter_byte_cnt, probed_size);

    RepetitionTester rt{
        "", real_byte_cnt, cpu_timer_freq, 15.f, false, false, 
        +[](uint64_t min_ticks, uint64_t, uint64_t, uint64_t,
            uint64_t processed_bytes, uint64_t cpu_timer_freq,
            uint64_t, uint64_t, uint64_t, bool, const char *) {

            long double const min_sec = (long double)min_ticks / cpu_timer_freq;
            long double const processed_gb =
                (long double)processed_bytes / (long double)(1u << 30);

            res = processed_gb / min_sec;
        }
    };

    rt.ReStart();
    do {
        rt.BeginTimeBlock();
        uint64_t byte_cnt = run_loop_load_npot(iter_byte_cnt, mem, probed_size);
        rt.EndTimeBlock();
        rt.ReportProcessedBytes(byte_cnt);
    } while (rt.Tick());

    return res;
}

int main()
{
    constexpr int c_min_tested_pot = 12; // 4kb
    constexpr int c_max_tested_pot = 29; // 512mb
    constexpr size_t c_pot_measurements = c_max_tested_pot - c_min_tested_pot + 1;
    constexpr size_t c_pot_intervals = c_pot_measurements - 1;
    constexpr size_t c_measurements_per_pot_gap_base = 8;

    constexpr size_t c_test_byte_count = (1 << 29) * 2; // 1gb

    init_os_process_state(g_os_proc_state);
    uint64_t cpu_timer_freq = measure_cpu_timer_freq(0.1l);

    char *mem;
    bool has_large_pages = try_enable_large_pages(g_os_proc_state);
    if (has_large_pages) {
        fprintf(stderr, "Trying large pages\n");
        mem = (char *)allocate_os_large_pages_memory(c_test_byte_count);
        if (mem) {
            fprintf(stderr, "Using large pages\n");
        } else {
            has_large_pages = false;
            fprintf(
                stderr,
                "Failed large page allocation, falling back to regular\n");
        }
    }

    if (!mem)
        mem = (char *)allocate_os_pages_memory(c_test_byte_count);

    if (!mem) {
        fprintf(stderr, "Allocation failed\n");
        return 2;
    }

    page_memory_in(mem, c_test_byte_count);
    
    {
        uint64_t pot_sizes[c_pot_measurements] = {};
        long double pot_results[c_pot_measurements] = {};
        fill_first_pass_sizes(pot_sizes, c_min_tested_pot, c_max_tested_pot);

        for (size_t i = c_min_tested_pot; i <= c_max_tested_pot; ++i) {
            pot_results[i - c_min_tested_pot] =
                probe_cache_size(c_test_byte_count, mem, 1 << i, cpu_timer_freq);
            fprintf(stderr, "prelim measurement %llu: %Lfgb/s at %ukb\n",
                i - c_min_tested_pot, pot_results[i - c_min_tested_pot],
                (1 << (i - 10)));
        }

        long double pot_deltas[c_pot_intervals] = {};
        struct {
            long double val; size_t id;
        } deltas_ranked[c_pot_intervals] = {};
        long double pot_total_variance = 0.0; // May not be the right word
        fprintf(stderr, "Deltas: ");
        for (size_t i = 0; i < c_pot_intervals; ++i) {
            long double delta = pot_results[i + 1] - pot_results[i];
            pot_deltas[i] = delta;
            deltas_ranked[i] = {delta, i};
            pot_total_variance += abs(pot_deltas[i]);
            fprintf(stderr, "%Lf ", pot_deltas[i]);
        }

        fprintf(stderr, "\nTotal variance = %Lf\n", pot_total_variance);

        // sort highest to lowest
        insertion_sort(
            deltas_ranked, deltas_ranked + c_pot_intervals, 
            [](const auto &s1, const auto &s2) { return abs(s1.val) > abs(s2.val); });

        fprintf(stderr, "Deltas sorted: ");
        for (size_t i = 0; i < c_pot_intervals; ++i)
            fprintf(stderr, "{%Lf, %llu} ", deltas_ranked[i].val, deltas_ranked[i].id);
        fprintf(stderr, "\n");

        printf("bytesize,gbps\n");

        for (size_t i = c_min_tested_pot; i < c_max_tested_pot; ++i) {
            size_t id = i - c_min_tested_pot;
            long double delta = abs(pot_deltas[id]);

            size_t rk;
            for (rk = 0; rk < c_pot_intervals; ++rk) {
                if (deltas_ranked[rk].id == id)
                    break;
            }
            assert(rk < c_pot_intervals);

            size_t desired_measurements;
            if (rk <= 4)
                desired_measurements = 16;
            else if (rk <= 7)
                desired_measurements = 8;
            else if (rk <= 10)
                desired_measurements = 4;
            else
                desired_measurements = 2;

            fprintf(
                stderr, "Interval [%d, %d]: rk = %llu, samples = %llu\n",
                1 << i, 1 << (i + 1), rk, desired_measurements);

            uint64_t base = 1 << i;
            uint64_t cap = 1 << (i + 1);
            uint64_t step = (cap - base) / desired_measurements;

            for (uint64_t sz = base; sz < cap; sz += step) {
                long double gbps =
                    probe_cache_size(c_test_byte_count, mem, sz, cpu_timer_freq);
                printf("%llu,%Lf\n", sz, gbps);
            }
        }
    }

    return 0;
}

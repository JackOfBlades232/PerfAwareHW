#include <repetition.hpp>
#include <profiling.hpp>
#include <memory.hpp>
#include <os.hpp>
#include <defs.hpp>
#include <algo.hpp>

extern "C"
{
extern u64 run_loop_load_npot(u64 count, char *ptr, u64 sz);
}

static f64 get_gbps_throughput_from_res(
    repetition_test_results_t const &results,
    u64 target_bytes, u64 cpu_timer_freq)
{
    f64 const min_sec = large_divide(results.min_ticks, cpu_timer_freq);
    f64 const processed_gb = f64(target_bytes) / f64(1u << 30);

    return processed_gb / min_sec;
}

static void fill_first_pass_sizes(u64 *sizes, int min_pot, int max_pot)
{
    for (int i = min_pot; i <= max_pot; ++i)
        *sizes++ = 1 << i;
}

static f64 probe_cache_size(
    u64 iter_byte_cnt, char *mem,
    u64 probed_size, u64 cpu_timer_freq)
{
    u64 const real_byte_cnt = round_up(iter_byte_cnt, probed_size);

    RepetitionTester rt{real_byte_cnt, cpu_timer_freq, 15.f, false};
    repetition_test_results_t results = {};

    rt.ReStart(results);
    do {
        rt.BeginTimeBlock();
        u64 byte_cnt = run_loop_load_npot(iter_byte_cnt, mem, probed_size);
        rt.EndTimeBlock();
        rt.ReportProcessedBytes(byte_cnt);
    } while (rt.Tick());

    return get_gbps_throughput_from_res(
        results, rt.GetTargetBytes(), cpu_timer_freq);
}

int main()
{
    constexpr int c_min_tested_pot = 12; // 4kb
    constexpr int c_max_tested_pot = 29; // 512mb
    constexpr usize c_pot_measurements = c_max_tested_pot - c_min_tested_pot + 1;
    constexpr usize c_pot_intervals = c_pot_measurements - 1;
    constexpr usize c_measurements_per_pot_gap_base = 8;

    constexpr usize c_test_byte_count = 1 << 30; // 1gb

    init_os_process_state(g_os_proc_state);
    u64 cpu_timer_freq = measure_cpu_timer_freq(0.1l);

    char *mem = nullptr;
    bool has_large_pages = try_enable_large_pages(g_os_proc_state);
    if (has_large_pages) {
        fprintf(stderr, "Trying large pages\n");
        mem = (char *)allocate_os_large_pages_memory(c_test_byte_count);
        if (mem) {
            fprintf(stderr, "Using large pages\n");
        } else {
            has_large_pages = false;
            fprintf(stderr,
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
        u64 pot_sizes[c_pot_measurements] = {};
        f64 pot_results[c_pot_measurements] = {};
        fill_first_pass_sizes(pot_sizes, c_min_tested_pot, c_max_tested_pot);

        for (usize i = c_min_tested_pot; i <= c_max_tested_pot; ++i) {
            pot_results[i - c_min_tested_pot] =
                probe_cache_size(c_test_byte_count, mem, 1 << i, cpu_timer_freq);
            fprintf(stderr, "prelim measurement %llu: %lfgb/s at %ukb\n",
                i - c_min_tested_pot, pot_results[i - c_min_tested_pot],
                (1 << (i - 10)));
        }

        f64 pot_deltas[c_pot_intervals] = {};
        struct {
            f64 val; usize id;
        } deltas_ranked[c_pot_intervals] = {};
        f64 pot_total_variance = 0.0; // May not be the right word
        fprintf(stderr, "Deltas: ");
        for (usize i = 0; i < c_pot_intervals; ++i) {
            f64 delta = pot_results[i + 1] - pot_results[i];
            pot_deltas[i] = delta;
            deltas_ranked[i] = {delta, i};
            pot_total_variance += abs(pot_deltas[i]);
            fprintf(stderr, "%lf ", pot_deltas[i]);
        }

        fprintf(stderr, "\nTotal variance = %lf\n", pot_total_variance);

        // sort highest to lowest
        insertion_sort(
            deltas_ranked, deltas_ranked + c_pot_intervals, 
            [](const auto &s1, const auto &s2) { return abs(s1.val) > abs(s2.val); });

        fprintf(stderr, "Deltas sorted: ");
        for (usize i = 0; i < c_pot_intervals; ++i)
            fprintf(stderr, "{%lf, %llu} ", deltas_ranked[i].val, deltas_ranked[i].id);
        fprintf(stderr, "\n");

        printf("bytesize,gbps\n");

        for (usize i = c_min_tested_pot; i < c_max_tested_pot; ++i) {
            usize id = i - c_min_tested_pot;
            f64 delta = abs(pot_deltas[id]);

            usize rk;
            for (rk = 0; rk < c_pot_intervals; ++rk) {
                if (deltas_ranked[rk].id == id)
                    break;
            }
            assert(rk < c_pot_intervals);

            usize desired_measurements;
            if (rk <= 4)
                desired_measurements = 16;
            else if (rk <= 7)
                desired_measurements = 8;
            else if (rk <= 10)
                desired_measurements = 4;
            else
                desired_measurements = 2;

            fprintf(stderr, "Interval [%d, %d]: rk = %llu, samples = %llu\n",
                1 << i, 1 << (i + 1), rk, desired_measurements);

            u64 base = 1 << i;
            u64 cap = 1 << (i + 1);
            u64 step = (cap - base) / desired_measurements;

            for (u64 sz = base; sz < cap; sz += step) {
                f64 gbps = probe_cache_size(c_test_byte_count, mem, sz, cpu_timer_freq);
                printf("%llu,%lf\n", sz, gbps);
            }
        }
    }

    return 0;
}

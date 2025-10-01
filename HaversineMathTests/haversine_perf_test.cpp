#include <haversine_state.hpp>
#include <haversine_calculation.hpp>
#include <haversine_validation.hpp>

#include <os.hpp>
#include <profiling.hpp>
#include <logging.hpp>
#include <repetition.hpp>

#define RT_STOP_TIME 10.f

struct tested_calc_func_t {
    void (*f)(haversine_state_t &);
    char const *name;
};

#define TEST_FUNC(f_) tested_calc_func_t{&f_, #f_}

int main(int argc, char **argv)
{
    if (argc < 2) {
        LOGERR("Usage: <program> [fnames...]");
        return 1;
    }

    char const *const *json_fns = &argv[1];
    int const json_fn_cnt = argc - 1;

    init_os_process_state(g_os_proc_state);
    u64 cpu_timer_freq = measure_cpu_timer_freq(0.1l);

    constexpr tested_calc_func_t c_test_funcs[] =
    {
        TEST_FUNC(calculate_haversine_distances_naive),
    };

    printf("File,Size");
    for (auto [_, name] : c_test_funcs)
        printf(",%s", name);
    printf("\n");

    for (int json_id = 0; json_id < json_fn_cnt; ++json_id) {
        char const *json_fn = json_fns[json_id];

        haversine_state_t state = {};
        if (!setup_haversine_state(state, json_fn))
            return 2;
        DEFER([&] { cleanup_haversine_state(state); });

        u64 const byte_count = state.pair_cnt * sizeof(point_pair_t);

        printf("%s,%lu", json_fn, byte_count);

        RepetitionTester rt{byte_count, cpu_timer_freq, RT_STOP_TIME, true};
        repetition_test_results_t results{};

        for (auto [f, name] : c_test_funcs) {
            rt.ReStart(results, byte_count);
            do {
                rt.BeginTimeBlock();
                (*f)(state);
                rt.EndTimeBlock();

                if (!validate_haversine_distances(state))
                    return 3;

                rt.ReportProcessedBytes(byte_count);
            } while (rt.Tick());

            {
                char namebuf[256];
                snprintf(
                    namebuf, sizeof(namebuf), "%s (%llukb)",
                    name, byte_count >> 10);
                print_reptest_results(
                    results, byte_count, cpu_timer_freq, namebuf, true);
            }

            printf(",%lf", gb_per_measure(
                large_divide(results.min_ticks, cpu_timer_freq), byte_count));
        }

        printf("\n");
    }
}

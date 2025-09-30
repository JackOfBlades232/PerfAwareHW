#include <repetition.hpp>
#include <profiling.hpp>
#include <memory.hpp>
#include <os.hpp>
#include <defs.hpp>

extern "C"
{
extern u64 run_write_loop(u64 count, char *ptr);
extern u64 run_empty_loop(u64 count, char *ptr);
extern u64 run_nop_loop(u64 count, char *ptr);
extern u64 run_nop3_loop(u64 count, char *ptr);
extern u64 run_3nop1_loop(u64 count, char *ptr);
extern u64 run_nop9_loop(u64 count, char *ptr);
extern u64 run_3nop3_loop(u64 count, char *ptr);
extern u64 run_9nop_loop(u64 count, char *ptr);
}

template <class TCallable>
static void run_test(
    TCallable &&tested, RepetitionTester &rt,
    repetition_test_results_t &results,
    char const *name, u64 cpu_timer_freq)
{
    rt.ReStart(results);
    do {
        rt.BeginTimeBlock();
        u64 byte_cnt = tested();
        rt.EndTimeBlock();

        rt.ReportProcessedBytes(byte_cnt);
    } while (rt.Tick());
    print_reptest_results(
        results, rt.GetTargetBytes(), cpu_timer_freq, name, true);
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: <program> [byte count]\n");
        return 1;
    }

    usize const byte_count = atol(argv[1]);

    init_os_process_state(g_os_proc_state);
    u64 cpu_timer_freq = measure_cpu_timer_freq(0.1l);

    RepetitionTester rt{byte_count, cpu_timer_freq, 10.f, false};
    repetition_test_results_t results = {};

    char *mem = (char *)allocate_os_pages_memory(byte_count);
    if (!mem) {
        fprintf(stderr, "Allocation failed\n");
        return 2;
    }

#define RUN_TEST(func_name_)                                       \
    run_test(                                                      \
        [mem, byte_count] { return func_name_(byte_count, mem); }, \
        rt, results, #func_name_, cpu_timer_freq)

    for (;;) {
        RUN_TEST(run_write_loop);
        RUN_TEST(run_empty_loop);
        RUN_TEST(run_nop_loop);
        RUN_TEST(run_nop3_loop);
        RUN_TEST(run_3nop1_loop);
        RUN_TEST(run_nop9_loop);
        RUN_TEST(run_3nop3_loop);
        RUN_TEST(run_9nop_loop);
    }

    free_os_pages_memory(mem, byte_count);
    return 0;
}

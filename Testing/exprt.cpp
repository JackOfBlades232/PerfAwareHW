#include <repetition.hpp>
#include <profiling.hpp>
#include <memory.hpp>
#include <os.hpp>
#include <util.hpp>

#include <cstdio>
#include <cstdlib>
#include <cstdint>

extern "C"
{
extern void run_loop_load(uint64_t count, char *ptr);
extern void run_loop_load2(uint64_t count, char *ptr);
extern void run_loop_load3(uint64_t count, char *ptr);
extern void run_loop_load4(uint64_t count, char *ptr);
extern void run_loop_load2xbyte(uint64_t count, char *ptr);
extern void run_loop_store(uint64_t count, char *ptr);
extern void run_loop_store2(uint64_t count, char *ptr);
extern void run_loop_store3(uint64_t count, char *ptr);
extern void run_loop_store4(uint64_t count, char *ptr);
extern void run_loop_store2xbyte(uint64_t count, char *ptr);
}

template <class TCallable>
static void run_test(TCallable &&tested, uint64_t iter_cnt,
                     RepetitionTester &rt,
                     repetition_test_results_t &results,
                     char const *name, uint64_t cpu_timer_freq)
{
    rt.ReStart(results);
    do {
        rt.BeginTimeBlock();
        tested();
        rt.EndTimeBlock();

        rt.ReportProcessedBytes(iter_cnt);
    } while (rt.Tick());
    print_reptest_results(results, cpu_timer_freq, name, true);
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: <program> [iter count]\n");
        return 1;
    }

    size_t const iter_count = atol(argv[1]);

    init_os_process_state(g_os_proc_state);
    uint64_t cpu_timer_freq = measure_cpu_timer_freq(0.1l);

    RepetitionTester rt{iter_count, cpu_timer_freq, 10.f, false};
    repetition_test_results_t results = {};

    char *mem = (char *)allocate_os_pages_memory(8);
    if (!mem) {
        fprintf(stderr, "Allocation failed\n");
        return 2;
    }

#define RUN_TEST(func_name_)                                            \
    run_test([mem, iter_count] { return func_name_(iter_count, mem); }, \
             iter_count, rt, results, #func_name_, cpu_timer_freq)

    for (;;) {
        RUN_TEST(run_loop_load);
        RUN_TEST(run_loop_load2);
        RUN_TEST(run_loop_load3);
        RUN_TEST(run_loop_load4);
        RUN_TEST(run_loop_load2xbyte);
        RUN_TEST(run_loop_store);
        RUN_TEST(run_loop_store2);
        RUN_TEST(run_loop_store3);
        RUN_TEST(run_loop_store4);
        RUN_TEST(run_loop_store2xbyte);
    }

    free_os_pages_memory(mem, 8);
    return 0;
}

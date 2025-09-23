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
extern uint64_t run_loop_load2x1(uint64_t count, char *ptr);
extern uint64_t run_loop_load2x2(uint64_t count, char *ptr);
extern uint64_t run_loop_load2x4(uint64_t count, char *ptr);
extern uint64_t run_loop_load2x8(uint64_t count, char *ptr);
extern uint64_t run_loop_load2x16(uint64_t count, char *ptr);
extern uint64_t run_loop_load2x32(uint64_t count, char *ptr);
extern uint64_t run_loop_load2x64(uint64_t count, char *ptr);
extern uint64_t run_loop_store2x1(uint64_t count, char *ptr);
extern uint64_t run_loop_store2x2(uint64_t count, char *ptr);
extern uint64_t run_loop_store2x4(uint64_t count, char *ptr);
extern uint64_t run_loop_store2x8(uint64_t count, char *ptr);
extern uint64_t run_loop_store2x16(uint64_t count, char *ptr);
extern uint64_t run_loop_store2x32(uint64_t count, char *ptr);
extern uint64_t run_loop_store2x64(uint64_t count, char *ptr);
}

template <class TCallable>
static void run_test(
    TCallable &&tested, RepetitionTester &rt,
    repetition_test_results_t &results,
    char const *name, uint64_t cpu_timer_freq)
{
    rt.ReStart(results);
    do {
        rt.BeginTimeBlock();
        uint64_t byte_cnt = tested();
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

    size_t const byte_count = atol(argv[1]);

    init_os_process_state(g_os_proc_state);
    uint64_t cpu_timer_freq = measure_cpu_timer_freq(0.1l);

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
        RUN_TEST(run_loop_load2x1);
        RUN_TEST(run_loop_load2x2);
        RUN_TEST(run_loop_load2x4);
        RUN_TEST(run_loop_load2x8);
        RUN_TEST(run_loop_load2x16);
        RUN_TEST(run_loop_load2x32);
        RUN_TEST(run_loop_load2x64);
        RUN_TEST(run_loop_store2x1);
        RUN_TEST(run_loop_store2x2);
        RUN_TEST(run_loop_store2x4);
        RUN_TEST(run_loop_store2x8);
        RUN_TEST(run_loop_store2x16);
        RUN_TEST(run_loop_store2x32);
        RUN_TEST(run_loop_store2x64);
    }

    free_os_pages_memory(mem, byte_count);
    return 0;
}

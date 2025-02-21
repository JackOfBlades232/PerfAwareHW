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
static void run_test(
    TCallable &&tested, uint64_t iter_cnt,
    RepetitionTester &rt, const char *name)
{
    rt.SetName(name);
    rt.ReStart();
    do {
        rt.BeginTimeBlock();
        tested();
        rt.EndTimeBlock();

        rt.ReportProcessedBytes(iter_cnt);
    } while (rt.Tick());
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: <program> [iter count]\n");
        return 1;
    }

    const size_t iter_count = atol(argv[1]);

    init_os_process_state(g_os_proc_state);
    uint64_t cpu_timer_freq = measure_cpu_timer_freq(0.1l);

    RepetitionTester rt{"Mem write loop", iter_count, cpu_timer_freq, 10.f, false, true};

    char *mem = (char *)allocate_os_pages_memory(8);
    if (!mem) {
        fprintf(stderr, "Allocation failed\n");
        return 2;
    }

#define RUN_TEST(func_name_) run_test([mem, iter_count] { return func_name_(iter_count, mem); }, iter_count, rt, #func_name_)

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

    free_os_pages_memory(mem, iter_count);
    return 0;
}

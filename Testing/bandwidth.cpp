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
extern uint64_t run_write_loop(uint64_t count, char *ptr);
extern uint64_t run_empty_loop(uint64_t count, char *ptr);
extern uint64_t run_nop_loop(uint64_t count, char *ptr);
extern uint64_t run_nop3_loop(uint64_t count, char *ptr);
extern uint64_t run_3nop1_loop(uint64_t count, char *ptr);
extern uint64_t run_nop9_loop(uint64_t count, char *ptr);
extern uint64_t run_3nop3_loop(uint64_t count, char *ptr);
extern uint64_t run_9nop_loop(uint64_t count, char *ptr);
}

template <class TCallable>
static void run_test(TCallable &&tested, RepetitionTester &rt, const char *name)
{
    rt.SetName(name);
    rt.ReStart();
    do {
        rt.BeginTimeBlock();
        uint64_t byte_cnt = tested();
        rt.EndTimeBlock();

        rt.ReportProcessedBytes(byte_cnt);
    } while (rt.Tick());
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: <program> [byte count]\n");
        return 1;
    }

    const size_t byte_count = atol(argv[1]);

    init_os_process_state(g_os_proc_state);
    uint64_t cpu_timer_freq = measure_cpu_timer_freq(0.1l);

    RepetitionTester rt{"Mem write loop", byte_count, cpu_timer_freq, 10.f, false, true};

    char *mem = (char *)allocate_os_pages_memory(byte_count);
    if (!mem) {
        fprintf(stderr, "Allocation failed\n");
        return 2;
    }

#define RUN_TEST(func_name_) run_test([mem, byte_count] { return func_name_(byte_count, mem); }, rt, #func_name_)

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

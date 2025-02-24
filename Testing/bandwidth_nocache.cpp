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

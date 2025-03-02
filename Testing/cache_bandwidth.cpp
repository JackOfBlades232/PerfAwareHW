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
extern uint64_t run_loop_load(uint64_t count, char *ptr, uint64_t mask);
extern uint64_t run_loop_store(uint64_t count, char *ptr, uint64_t mask);
}

uint64_t kb(uint64_t cnt)
{
    return cnt << 10;
}

uint64_t mb(uint64_t cnt)
{
    return cnt << 20;
}

uint64_t gb(uint64_t cnt)
{
    return cnt << 30;
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

    if (byte_count < mb(256)) {
        fprintf(stderr, "Minimal byte count for the test suite is 256mb\n");
        return 1;
    }

    init_os_process_state(g_os_proc_state);
    uint64_t cpu_timer_freq = measure_cpu_timer_freq(0.1l);

    RepetitionTester rt{"Mem write loop", byte_count, cpu_timer_freq, 10.f, false, true};

    char *mem = (char *)allocate_os_pages_memory(byte_count);
    if (!mem) {
        fprintf(stderr, "Allocation failed\n");
        return 2;
    }

    // Linux needs data in pages for them to not be mapped to null page
    for (char *p = mem; p != mem + byte_count; ++p)
        *p = char(p - mem);

#define RUN_TEST(func_name_, size_) \
    run_test([mem, byte_count] { return func_name_(byte_count, mem, (size_) - 1); }, rt, #func_name_ "_" #size_)

    for (;;) {
        RUN_TEST(run_loop_load, kb(4));
        RUN_TEST(run_loop_load, kb(16));
        RUN_TEST(run_loop_load, kb(32));
        RUN_TEST(run_loop_load, kb(64));
        RUN_TEST(run_loop_load, kb(128));
        RUN_TEST(run_loop_load, kb(256));
        RUN_TEST(run_loop_load, kb(512));
        RUN_TEST(run_loop_load, mb(1));
        RUN_TEST(run_loop_load, mb(2));
        RUN_TEST(run_loop_load, mb(4));
        RUN_TEST(run_loop_load, mb(8));
        RUN_TEST(run_loop_load, mb(16));
        RUN_TEST(run_loop_load, mb(32));
        RUN_TEST(run_loop_load, mb(64));
        RUN_TEST(run_loop_load, mb(128));
        RUN_TEST(run_loop_load, mb(256));
        RUN_TEST(run_loop_store, kb(4));
        RUN_TEST(run_loop_store, kb(16));
        RUN_TEST(run_loop_store, kb(32));
        RUN_TEST(run_loop_store, kb(64));
        RUN_TEST(run_loop_store, kb(128));
        RUN_TEST(run_loop_store, kb(256));
        RUN_TEST(run_loop_store, kb(512));
        RUN_TEST(run_loop_store, mb(1));
        RUN_TEST(run_loop_store, mb(2));
        RUN_TEST(run_loop_store, mb(4));
        RUN_TEST(run_loop_store, mb(8));
        RUN_TEST(run_loop_store, mb(16));
        RUN_TEST(run_loop_store, mb(32));
        RUN_TEST(run_loop_store, mb(64));
        RUN_TEST(run_loop_store, mb(128));
        RUN_TEST(run_loop_store, mb(256));
    }

    free_os_pages_memory(mem, byte_count);
    return 0;
}

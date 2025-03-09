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
extern uint64_t run_loop_load_pot(uint64_t count, char *ptr, uint64_t mask);
extern uint64_t run_loop_store_pot(uint64_t count, char *ptr, uint64_t mask);
extern uint64_t run_loop_load_npot(uint64_t count, char *ptr, uint64_t sz);
extern uint64_t run_loop_store_npot(uint64_t count, char *ptr, uint64_t sz);
}

uint64_t kb(uint64_t cnt)
{
    return cnt << 10;
}

uint64_t mb(uint64_t cnt)
{
    return cnt << 20;
}

template <class TCallable>
static void run_test(TCallable &&tested, RepetitionTester &rt,
                     const char *name, uint64_t target_bytes)
{
    rt.SetName(name);
    rt.SetTargetBytes(target_bytes);
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
    if (byte_count % mb(256) != 0) {
        fprintf(stderr, "Byte count must be divisible by 256mb\n");
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

#define RUN_TEST_POT(func_name_, size_)                            \
    run_test([mem, byte_count] {                                   \
        return func_name_ ## _pot(byte_count, mem, (size_) - 1);   \
    }, rt, #func_name_ "_" #size_, round_up(byte_count, (size_)))
#define RUN_TEST_NPOT(func_name_, size_)                           \
    run_test([mem, byte_count] {                                   \
        return func_name_ ## _npot(byte_count, mem, (size_));      \
    }, rt, #func_name_ "_" #size_, round_up(byte_count, (size_)))
#define RUN_TEST_NPOT_OFF(func_name_, size_, off_)                     \
    run_test([mem, byte_count] {                                       \
        return func_name_ ## _npot(byte_count, mem + (off_), (size_)); \
    }, rt, #func_name_ "_" #size_ "_" #off_, round_up(byte_count, (size_)))

    for (;;) {
        RUN_TEST_NPOT(run_loop_load, kb(48));
        RUN_TEST_NPOT_OFF(run_loop_load, kb(48), 1);
        RUN_TEST_NPOT_OFF(run_loop_load, kb(48), 3);
        RUN_TEST_NPOT_OFF(run_loop_load, kb(48), 7);
        RUN_TEST_NPOT_OFF(run_loop_load, kb(48), 15);
        RUN_TEST_NPOT_OFF(run_loop_load, kb(48), 31);
        RUN_TEST_NPOT_OFF(run_loop_load, kb(48), 63);
        RUN_TEST_NPOT(run_loop_load, kb(52));
        RUN_TEST_NPOT_OFF(run_loop_load, kb(52), 1);
        RUN_TEST_NPOT_OFF(run_loop_load, kb(52), 3);
        RUN_TEST_NPOT_OFF(run_loop_load, kb(52), 7);
        RUN_TEST_NPOT_OFF(run_loop_load, kb(52), 15);
        RUN_TEST_NPOT_OFF(run_loop_load, kb(52), 31);
        RUN_TEST_NPOT_OFF(run_loop_load, kb(52), 63);
        RUN_TEST_NPOT(run_loop_load, mb(1) + kb(256));
        RUN_TEST_NPOT_OFF(run_loop_load, mb(1) + kb(256), 1);
        RUN_TEST_NPOT_OFF(run_loop_load, mb(1) + kb(256), 3);
        RUN_TEST_NPOT_OFF(run_loop_load, mb(1) + kb(256), 7);
        RUN_TEST_NPOT_OFF(run_loop_load, mb(1) + kb(256), 15);
        RUN_TEST_NPOT_OFF(run_loop_load, mb(1) + kb(256), 31);
        RUN_TEST_NPOT_OFF(run_loop_load, mb(1) + kb(256), 63);
        RUN_TEST_NPOT(run_loop_load, mb(1) + kb(320));
        RUN_TEST_NPOT_OFF(run_loop_load, mb(1) + kb(320), 1);
        RUN_TEST_NPOT_OFF(run_loop_load, mb(1) + kb(320), 3);
        RUN_TEST_NPOT_OFF(run_loop_load, mb(1) + kb(320), 7);
        RUN_TEST_NPOT_OFF(run_loop_load, mb(1) + kb(320), 15);
        RUN_TEST_NPOT_OFF(run_loop_load, mb(1) + kb(320), 31);
        RUN_TEST_NPOT_OFF(run_loop_load, mb(1) + kb(320), 63);
        RUN_TEST_NPOT(run_loop_load, mb(1) + kb(384));
        RUN_TEST_NPOT_OFF(run_loop_load, mb(1) + kb(384), 1);
        RUN_TEST_NPOT_OFF(run_loop_load, mb(1) + kb(384), 3);
        RUN_TEST_NPOT_OFF(run_loop_load, mb(1) + kb(384), 7);
        RUN_TEST_NPOT_OFF(run_loop_load, mb(1) + kb(384), 15);
        RUN_TEST_NPOT_OFF(run_loop_load, mb(1) + kb(384), 31);
        RUN_TEST_NPOT_OFF(run_loop_load, mb(1) + kb(384), 63);
        RUN_TEST_NPOT(run_loop_load, mb(12));
        RUN_TEST_NPOT_OFF(run_loop_load, mb(12), 1);
        RUN_TEST_NPOT_OFF(run_loop_load, mb(12), 3);
        RUN_TEST_NPOT_OFF(run_loop_load, mb(12), 7);
        RUN_TEST_NPOT_OFF(run_loop_load, mb(12), 15);
        RUN_TEST_NPOT_OFF(run_loop_load, mb(12), 31);
        RUN_TEST_NPOT_OFF(run_loop_load, mb(12), 63);
        RUN_TEST_NPOT(run_loop_load, mb(14));
        RUN_TEST_NPOT_OFF(run_loop_load, mb(14), 1);
        RUN_TEST_NPOT_OFF(run_loop_load, mb(14), 3);
        RUN_TEST_NPOT_OFF(run_loop_load, mb(14), 7);
        RUN_TEST_NPOT_OFF(run_loop_load, mb(14), 15);
        RUN_TEST_NPOT_OFF(run_loop_load, mb(14), 31);
        RUN_TEST_NPOT_OFF(run_loop_load, mb(14), 63);

        RUN_TEST_POT(run_loop_load, kb(4));
        RUN_TEST_POT(run_loop_load, kb(16));
        RUN_TEST_POT(run_loop_load, kb(32));
        RUN_TEST_NPOT(run_loop_load, kb(36));
        RUN_TEST_NPOT(run_loop_load, kb(40));
        RUN_TEST_NPOT(run_loop_load, kb(44));
        RUN_TEST_NPOT(run_loop_load, kb(48));
        RUN_TEST_NPOT(run_loop_load, kb(52));
        RUN_TEST_NPOT(run_loop_load, kb(56));
        RUN_TEST_NPOT(run_loop_load, kb(60));
        RUN_TEST_POT(run_loop_load, kb(64));
        RUN_TEST_POT(run_loop_load, kb(128));
        RUN_TEST_POT(run_loop_load, kb(256));
        RUN_TEST_POT(run_loop_load, kb(512));
        RUN_TEST_NPOT(run_loop_load, kb(640));
        RUN_TEST_NPOT(run_loop_load, kb(768));
        RUN_TEST_NPOT(run_loop_load, kb(896));
        RUN_TEST_POT(run_loop_load, mb(1));
        RUN_TEST_NPOT(run_loop_load, mb(1) + kb(64));
        RUN_TEST_NPOT(run_loop_load, mb(1) + kb(128));
        RUN_TEST_NPOT(run_loop_load, mb(1) + kb(192));
        RUN_TEST_NPOT(run_loop_load, mb(1) + kb(256));
        RUN_TEST_NPOT(run_loop_load, mb(1) + kb(320));
        RUN_TEST_NPOT(run_loop_load, mb(1) + kb(384));
        RUN_TEST_NPOT(run_loop_load, mb(1) + kb(448));
        RUN_TEST_NPOT(run_loop_load, mb(1) + kb(512));
        RUN_TEST_NPOT(run_loop_load, mb(1) + kb(640));
        RUN_TEST_NPOT(run_loop_load, mb(1) + kb(768));
        RUN_TEST_NPOT(run_loop_load, mb(1) + kb(896));
        RUN_TEST_POT(run_loop_load, mb(2));
        RUN_TEST_POT(run_loop_load, mb(4));
        RUN_TEST_POT(run_loop_load, mb(8));
        RUN_TEST_NPOT(run_loop_load, mb(10));
        RUN_TEST_NPOT(run_loop_load, mb(12));
        RUN_TEST_NPOT(run_loop_load, mb(14));
        RUN_TEST_POT(run_loop_load, mb(16));
        RUN_TEST_NPOT(run_loop_load, mb(18));
        RUN_TEST_NPOT(run_loop_load, mb(20));
        RUN_TEST_NPOT(run_loop_load, mb(22));
        RUN_TEST_NPOT(run_loop_load, mb(24));
        RUN_TEST_NPOT(run_loop_load, mb(26));
        RUN_TEST_NPOT(run_loop_load, mb(28));
        RUN_TEST_NPOT(run_loop_load, mb(30));
        RUN_TEST_POT(run_loop_load, mb(32));
        RUN_TEST_POT(run_loop_load, mb(64));
        RUN_TEST_POT(run_loop_load, mb(128));
        RUN_TEST_POT(run_loop_load, mb(256));

    }

    free_os_pages_memory(mem, byte_count);
    return 0;
}

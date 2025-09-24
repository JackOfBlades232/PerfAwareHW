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
static void run_test(
    TCallable &&tested, uint64_t target_bytes, RepetitionTester &rt,
    char const *name, uint64_t cpu_timer_freq, bool write_csv)
{
    repetition_test_results_t results{};

    rt.ReStart(results, target_bytes);
    do {
        rt.BeginTimeBlock();
        uint64_t byte_cnt = tested();
        rt.EndTimeBlock();
        rt.ReportProcessedBytes(byte_cnt);
    } while (rt.Tick());

    if (write_csv)
        print_best_bandwidth_csv(results, target_bytes, cpu_timer_freq, name);
    else
        print_reptest_results(results, target_bytes, cpu_timer_freq, name, true);
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: <program> [byte count] (csv)\n");
        return 1;
    }

    size_t const byte_count = atol(argv[1]);
    bool const write_csv = argc > 2 && streq(argv[2], "csv");

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

    RepetitionTester rt{byte_count, cpu_timer_freq, 10.f, false};

    char *mem = (char *)allocate_os_pages_memory(byte_count);
    if (!mem) {
        fprintf(stderr, "Allocation failed\n");
        return 2;
    }

    page_memory_in(mem, byte_count);

    if (write_csv)
        printf("name,bytes,seconds,gb/s\n");

#define RUN_TEST_POT(func_name_, size_)                            \
    run_test([mem, byte_count] {                                   \
        return func_name_ ## _pot(byte_count, mem, (size_) - 1);   \
    }, round_up(byte_count, (size_)), rt, #func_name_ "_" #size_, cpu_timer_freq, write_csv)
#define RUN_TEST_NPOT(func_name_, size_)                           \
    run_test([mem, byte_count] {                                   \
        return func_name_ ## _npot(byte_count, mem, (size_));      \
    }, round_up(byte_count, (size_)), rt, #func_name_ "_" #size_, cpu_timer_freq, write_csv)
#define RUN_TEST_NPOT_OFF(func_name_, size_, off_)                     \
    run_test([mem, byte_count] {                                       \
        return func_name_ ## _npot(byte_count, mem + (off_), (size_)); \
    }, round_up(byte_count, (size_)), rt, #func_name_ "_" #size_ "_" #off_, cpu_timer_freq, write_csv)

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

        if (write_csv)
            break;
    }

    free_os_pages_memory(mem, byte_count);
    return 0;
}

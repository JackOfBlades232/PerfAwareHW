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
        rt.OverrideTargetBytes(byte_cnt);
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

    RepetitionTester rt{
        "Mem write loop", byte_count, cpu_timer_freq, 10.f, false, true,
        +[](uint64_t min_ticks, uint64_t, uint64_t, uint64_t test_count,
            uint64_t processed_bytes, uint64_t cpu_timer_freq,
            uint64_t, uint64_t, uint64_t, bool, const char *) {

            // @TODO: pull out these computations
            constexpr long double c_bytes_in_gb = (long double)(1u << 30);
            constexpr long double c_kb_in_gb = (long double)(1u << 20);
            auto gb_per_measure = [](long double measure, uint64_t bytes) {
                const long double gb = (long double)bytes / c_bytes_in_gb; 
                return gb / measure;
            };

            const long double min_sec = (long double)min_ticks / cpu_timer_freq;
            printf(",%Lf\n", gb_per_measure(min_sec, processed_bytes));
        }};

    char *mem = (char *)allocate_os_pages_memory(byte_count);
    if (!mem) {
        fprintf(stderr, "Allocation failed\n");
        return 2;
    }

    // Linux needs data in pages for them to not be mapped to null page
    for (char *p = mem; p != mem + byte_count; ++p)
        *p = char(p - mem);

    printf("bytes,gbps\n");

#define RUN_TEST_POT(func_name_, size_)                              \
    do {                                                             \
        printf("%llu", (size_));                                     \
        run_test([mem, byte_count] {                                 \
            return func_name_ ## _pot(byte_count, mem, (size_) - 1); \
        }, rt, #func_name_ "_" #size_);                              \
    } while (0)
#define RUN_TEST_NPOT(func_name_, size_)                          \
    do {                                                          \
        printf("%llu", (size_));                                  \
        run_test([mem, byte_count] {                              \
            return func_name_ ## _npot(byte_count, mem, (size_)); \
        }, rt, #func_name_ "_" #size_);                           \
    } while (0)

    for (;;) {
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

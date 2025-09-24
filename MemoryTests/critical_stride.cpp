#include <repetition.hpp>
#include <profiling.hpp>
#include <memory.hpp>
#include <os.hpp>
#include <util.hpp>

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cassert>

extern "C"
{
extern uint64_t run_loop_load_npot_strided(
    uint64_t total_count, char *ptr, uint64_t count, uint64_t stride);
}

uint64_t kb(uint64_t cnt)
{
    return cnt << 10;
}

uint64_t mb(uint64_t cnt)
{
    return cnt << 20;
}

uint64_t clines(uint64_t cnt)
{
    return cnt << 6;
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
        uint64_t cline_count = tested();
        rt.EndTimeBlock();
        rt.ReportProcessedBytes(clines(cline_count));
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

#define RUN_TEST(size_, stride_)                                            \
    do {                                                                    \
        assert((stride_) % clines(1) == 0);                                 \
        assert((size_) % clines(1) == 0);                                   \
        auto const real_bc = round_up(byte_count, (size_));                 \
        run_test(                                                           \
            [mem, real_bc] {                                                \
                return run_loop_load_npot_strided(                          \
                    real_bc / clines(1), mem,                               \
                    ((size_) / clines(1)), (stride_));                      \
            },                                                              \
            real_bc, rt, #size_ " | " #stride_, cpu_timer_freq, write_csv); \
    } while (0)

    for (;;) {
        // Base
        RUN_TEST(kb(48), clines(1));

        // Critical striding
        RUN_TEST(kb(48), clines(2));
        RUN_TEST(kb(48), clines(4));
        RUN_TEST(kb(48), clines(8));
        RUN_TEST(kb(48), clines(16));
        RUN_TEST(kb(48), clines(32));
        RUN_TEST(kb(48), clines(64));
        RUN_TEST(kb(48), clines(128));
        RUN_TEST(kb(48), clines(256));
        RUN_TEST(kb(512), clines(1));
        RUN_TEST(kb(512), clines(2));
        RUN_TEST(kb(512), clines(4));
        RUN_TEST(kb(512), clines(8));
        RUN_TEST(kb(512), clines(16));
        RUN_TEST(kb(512), clines(32));
        RUN_TEST(kb(512), clines(64));
        RUN_TEST(kb(512), clines(128));
        RUN_TEST(kb(512), clines(256));
        RUN_TEST(kb(512), clines(512));
        RUN_TEST(mb(4), clines(1));
        RUN_TEST(mb(4), clines(2));
        RUN_TEST(mb(4), clines(4));
        RUN_TEST(mb(4), clines(8));
        RUN_TEST(mb(4), clines(16));
        RUN_TEST(mb(4), clines(32));
        RUN_TEST(mb(4), clines(64));
        RUN_TEST(mb(4), clines(128));
        RUN_TEST(mb(4), clines(256));

        // Slanted to regain speed
        RUN_TEST(kb(48), clines(2) + clines(1));
        RUN_TEST(kb(48), clines(4) + clines(1));
        RUN_TEST(kb(48), clines(8) + clines(1));
        RUN_TEST(kb(48), clines(16) + clines(1));
        RUN_TEST(kb(48), clines(32) + clines(1));
        RUN_TEST(kb(48), clines(64) + clines(1));
        RUN_TEST(kb(48), clines(128) + clines(1));
        RUN_TEST(kb(48), clines(256) + clines(1));
        RUN_TEST(kb(512), clines(2) + clines(1));
        RUN_TEST(kb(512), clines(4) + clines(1));
        RUN_TEST(kb(512), clines(8) + clines(1));
        RUN_TEST(kb(512), clines(16) + clines(1));
        RUN_TEST(kb(512), clines(32) + clines(1));
        RUN_TEST(kb(512), clines(64) + clines(1));
        RUN_TEST(kb(512), clines(128) + clines(1));
        RUN_TEST(kb(512), clines(256) + clines(1));
        RUN_TEST(kb(512), clines(512) + clines(1));
        RUN_TEST(mb(4), clines(2) + clines(1));
        RUN_TEST(mb(4), clines(4) + clines(1));
        RUN_TEST(mb(4), clines(8) + clines(1));
        RUN_TEST(mb(4), clines(16) + clines(1));
        RUN_TEST(mb(4), clines(32) + clines(1));
        RUN_TEST(mb(4), clines(64) + clines(1));
        RUN_TEST(mb(4), clines(128) + clines(1));

        if (write_csv)
            break;
    }

    free_os_pages_memory(mem, byte_count);
    return 0;
}


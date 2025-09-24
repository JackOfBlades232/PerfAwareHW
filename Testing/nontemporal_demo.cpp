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
extern void memcpy512(void *to, void *from, uint64_t bytecnt);
extern void memcpy512_nt(void *to, void *from, uint64_t bytecnt);
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
        uint64_t bytes_cnt = tested();
        rt.EndTimeBlock();
        rt.ReportProcessedBytes(bytes_cnt);
    } while (rt.Tick());

    if (write_csv)
        print_best_bandwidth_csv(results, target_bytes, cpu_timer_freq, name);
    else
        print_reptest_results(results, target_bytes, cpu_timer_freq, name, true);
}

int main(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "Usage: <program> [pattern kb] [pattern count] (csv)\n");
        return 1;
    }

    size_t const pattern_byte_count = atol(argv[1]) << 10;
    size_t const pattern_count = atol(argv[2]);
    size_t const output_byte_count = pattern_count * pattern_byte_count;
    bool const write_csv = argc > 3 && streq(argv[3], "csv");

    init_os_process_state(g_os_proc_state);
    uint64_t cpu_timer_freq = measure_cpu_timer_freq(0.1l);

    RepetitionTester rt{output_byte_count, cpu_timer_freq, 10.f, false};

    char *pattern_mem = (char *)allocate_os_pages_memory(pattern_byte_count);
    char *output_mem = (char *)allocate_os_pages_memory(output_byte_count);
    if (!pattern_mem || !output_mem) {
        fprintf(stderr, "Allocation failed\n");
        return 2;
    }

    page_memory_in(output_mem, output_byte_count);
    for (size_t i = 0; i < pattern_byte_count; ++i)
        pattern_mem[i] = "deadbeef"[i & 7];

    if (write_csv)
        printf("name,bytes,seconds,gb/s\n");

    run_test(
        [&] {
            char *dst = output_mem;
            for (size_t i = 0; i < pattern_count; ++i, dst += pattern_byte_count)
                memcpy512(dst, pattern_mem, pattern_byte_count);
            return output_byte_count;
        },
        output_byte_count, rt, "regular stores", cpu_timer_freq, write_csv);
    run_test(
        [&] {
            char *dst = output_mem;
            for (size_t i = 0; i < pattern_count; ++i, dst += pattern_byte_count)
                memcpy512_nt(dst, pattern_mem, pattern_byte_count);
            return output_byte_count;
        },
        output_byte_count, rt, "nontemporal stores", cpu_timer_freq, write_csv);

    free_os_pages_memory(pattern_mem, pattern_byte_count);
    free_os_pages_memory(output_mem, output_byte_count);
    return 0;
}


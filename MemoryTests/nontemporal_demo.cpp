#include <repetition.hpp>
#include <profiling.hpp>
#include <memory.hpp>
#include <os.hpp>
#include <defs.hpp>

#define RT_STOP_TIME 10.f

extern "C"
{
extern void memcpy512(void *to, void const *from, u64 bytecnt);
extern void memcpy512_nt(void *to, void const *from, u64 bytecnt);
}

using test_func_t = void (*)(void *, void const *, u64);

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: <program> [pattern kb] (csv)\n");
        return 1;
    }

    constexpr usize c_pattern_counts[] = { 64, 256, 1024, 4096, 16384, 65536, 262144, 1048576 };
    constexpr usize c_scale_count = sizeof(c_pattern_counts) / sizeof(c_pattern_counts[0]);

    constexpr test_func_t c_test_funcs[] = { &memcpy512, &memcpy512_nt };
    constexpr usize c_test_func_count = sizeof(c_test_funcs) / sizeof(c_test_funcs[0]);

    constexpr char const *c_test_func_names[c_test_func_count] = { "Regular stores", "Nontemporal stores" };

    usize const pattern_byte_count = atol(argv[1]) << 10;
    usize const output_byte_count = c_pattern_counts[c_scale_count - 1] * pattern_byte_count;
    bool const write_csv = argc > 2 && streq(argv[2], "csv");

    init_os_process_state(g_os_proc_state);
    u64 cpu_timer_freq = measure_cpu_timer_freq(0.1l);

    char *pattern_mem = (char *)allocate_os_pages_memory(pattern_byte_count);
    char *output_mem = (char *)allocate_os_pages_memory(output_byte_count);
    if (!pattern_mem || !output_mem) {
        fprintf(stderr, "Allocation failed\n");
        return 2;
    }

    page_memory_in(output_mem, output_byte_count);
    for (usize i = 0; i < pattern_byte_count; ++i)
        pattern_mem[i] = "deadbeef"[i & 7];

    if (write_csv) {
        printf("bytes");
        for (char const *name : c_test_func_names)
            printf(",%s", name);
        printf("\n");
    }

    RepetitionTester rt{output_byte_count, cpu_timer_freq, RT_STOP_TIME, false};
    repetition_test_results_t results{};

    for (usize count : c_pattern_counts) {
        if (write_csv)
            printf("%llu", pattern_byte_count * count);
        for (usize fi = 0; test_func_t func : c_test_funcs) {
            rt.ReStart(results, output_byte_count);
            do {
                rt.BeginTimeBlock();
                {
                    for (usize i = 0, end = output_byte_count / (pattern_byte_count * count); i < end; ++i) {
                        char *dst = output_mem;
                        for (usize j = 0; j < count; ++j, dst += pattern_byte_count)
                            (*func)(dst, pattern_mem, pattern_byte_count);
                    }
                }
                rt.EndTimeBlock();
                rt.ReportProcessedBytes(output_byte_count);
            } while (rt.Tick());

            if (write_csv) {
                printf(",%lf", gb_per_measure(
                    large_divide(results.min_ticks, cpu_timer_freq),
                    output_byte_count));
            } else {
                char namebuf[256];
                snprintf(
                    namebuf, sizeof(namebuf), "%s (%llukb repeated over %llukb)",
                    c_test_func_names[fi], pattern_byte_count >> 10,
                    (count * pattern_byte_count) >> 10);
                print_reptest_results(results, output_byte_count, cpu_timer_freq, namebuf, true);
            }

            ++fi;
        }

        if (write_csv)
            printf("\n");
    }

    free_os_pages_memory(pattern_mem, pattern_byte_count);
    free_os_pages_memory(output_mem, output_byte_count);
    return 0;
}


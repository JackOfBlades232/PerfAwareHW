#include <repetition.hpp>
#include <profiling.hpp>
#include <memory.hpp>
#include <os.hpp>
#include <defs.hpp>

#define RT_STOP_TIME 10.f

extern "C"
{
extern u64 ll_foreach(void const *ll, u64 bytecnt, u64 itercnt);
extern u64 ll_foreach_prefetch(void const *ll, u64 bytecnt, u64 itercnt);
}

using test_func_t = u64 (*)(void const *, u64, u64);

struct alignas(clines(1)) node_t {
    node_t *next = nullptr;
    f32 data[14] = {};
};

static_assert(sizeof(node_t) == clines(1));

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: <program> [cline count] (csv)\n");
        return 1;
    }

    constexpr usize c_work_iter_counts[] = { 1, 2, 4, 8, 32, 64, 128, 192, 256, 512, 1024 };

    constexpr test_func_t c_test_funcs[] = { &ll_foreach, &ll_foreach_prefetch };
    constexpr usize c_test_func_count = sizeof(c_test_funcs) / sizeof(c_test_funcs[0]);

    constexpr char const *c_test_func_names[c_test_func_count] =
        { "Regular loop", "Loop with next node prefetch" };

    usize const count = atol(argv[1]);
    bool const write_csv = argc > 2 && streq(argv[2], "csv");

    usize const byte_count = clines(count);

    init_os_process_state(g_os_proc_state);
    u64 cpu_timer_freq = measure_cpu_timer_freq(0.1l);

    srand(time(nullptr));

    node_t *nodes = (node_t *)allocate_os_pages_memory(byte_count);
    if (!nodes) {
        fprintf(stderr, "Allocation failed\n");
        return 2;
    }

    page_memory_in(nodes, byte_count);

    node_t *head = nullptr;

    {
        int *index_order = (int *)allocate_os_pages_memory(count * sizeof(int));
        if (!index_order) {
            fprintf(stderr, "Allocation failed\n");
            free_os_pages_memory(nodes, byte_count);
            return 2;
        }

        for (usize i = 0; i < count; ++i)
            index_order[i] = int(i);
        for (usize i = 0; i < count - 1; ++i) {
            usize swi = rand() % (count - i) + i;
            assert(swi >= i && swi < count);
            swp(index_order[i], index_order[swi]);
        }

        for (usize i = 0; i < count - 1; ++i)
            nodes[index_order[i]].next = &nodes[index_order[i + 1]];
        nodes[index_order[count - 1]].next = nullptr;
        head = &nodes[index_order[0]];

        free_os_pages_memory(index_order, count * sizeof(int));
    }

    RepetitionTester rt{byte_count, cpu_timer_freq, RT_STOP_TIME, true};
    repetition_test_results_t results{};

    if (write_csv) {
        printf("iterations");
        for (char const *name : c_test_func_names)
            printf(",%s", name);
        printf("\n");
    }

    for (usize iters : c_work_iter_counts) {
        if (write_csv)
            printf("%llu", iters);
        for (usize fi = 0; test_func_t func : c_test_funcs) {
            rt.ReStart(results, byte_count);
            do {
                rt.BeginTimeBlock();
                u64 real_byte_count = (*func)(head, byte_count, iters);
                rt.EndTimeBlock();
                rt.ReportProcessedBytes(real_byte_count);
            } while (rt.Tick());

            if (write_csv) {
                printf(",%lf", gb_per_measure(
                    large_divide(results.min_ticks, cpu_timer_freq), byte_count));
            }

            {
                char namebuf[256];
                snprintf(
                    namebuf, sizeof(namebuf), "%s (%llu work loop iterations)",
                    c_test_func_names[fi], iters);
                print_reptest_results(results, byte_count, cpu_timer_freq, namebuf, true);
            }

            ++fi;
        }

        if (write_csv)
            printf("\n");
    }

    free_os_pages_memory(nodes, byte_count);
    return 0;
}


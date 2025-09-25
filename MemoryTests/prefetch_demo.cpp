#define RT_PRINT(fmt_, ...) fprintf(stderr, fmt_, ##__VA_ARGS__)
#define RT_PRINTLN(fmt_, ...) fprintf(stderr, fmt_ "\n", ##__VA_ARGS__)
#define RT_CLEAR(count_)                         \
    do {                                         \
        for (size_t i_ = 0; i_ < (count_); ++i_) \
            fprintf(stderr, "\b");               \
    } while (0)

#include <repetition.hpp>
#include <profiling.hpp>
#include <memory.hpp>
#include <os.hpp>
#include <util.hpp>

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cassert>

#define RT_STOP_TIME 10.f

extern "C"
{
extern uint64_t ll_foreach(const void *ll, uint64_t bytecnt, uint64_t itercnt);
extern uint64_t ll_foreach_prefetch(const void *ll, uint64_t bytecnt, uint64_t itercnt);
}

using test_func_t = uint64_t (*)(const void *, uint64_t, uint64_t);

constexpr uint64_t clines(uint64_t cnt)
{
    return cnt << 6;
}

struct alignas(clines(1)) node_t {
    node_t *next = nullptr;
    float data[14] = {};
};

static_assert(sizeof(node_t) == clines(1));

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: <program> [cline count] (csv)\n");
        return 1;
    }

    constexpr size_t c_work_iter_counts[] = { 1, 2, 4, 8, 32, 64, 128, 192, 256, 512, 1024 };

    constexpr test_func_t c_test_funcs[] = { &ll_foreach, &ll_foreach_prefetch };
    constexpr size_t c_test_func_count = sizeof(c_test_funcs) / sizeof(c_test_funcs[0]);

    constexpr char const *c_test_func_names[c_test_func_count] =
        { "Regular loop", "Loop with next node prefetch" };

    size_t const count = atol(argv[1]);
    bool const write_csv = argc > 2 && streq(argv[2], "csv");

    size_t const byte_count = clines(count);

    init_os_process_state(g_os_proc_state);
    uint64_t cpu_timer_freq = measure_cpu_timer_freq(0.1l);

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

        for (size_t i = 0; i < count; ++i)
            index_order[i] = int(i);
        for (size_t i = 0; i < count - 1; ++i) {
            size_t swi = rand() % (count - i) + i;
            assert(swi >= i && swi < count);
            swp(index_order[i], index_order[swi]);
        }

        for (size_t i = 0; i < count - 1; ++i)
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

    for (size_t iters : c_work_iter_counts) {
        if (write_csv)
            printf("%llu", iters);
        for (size_t fi = 0; test_func_t func : c_test_funcs) {
            rt.ReStart(results, byte_count);
            do {
                rt.BeginTimeBlock();
                uint64_t real_byte_count = (*func)(head, byte_count, iters);
                rt.EndTimeBlock();
                rt.ReportProcessedBytes(real_byte_count);
            } while (rt.Tick());

            if (write_csv) {
                printf(",%Lf", gb_per_measure(
                    (long double)results.min_ticks / cpu_timer_freq, byte_count));
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


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
extern uint64_t calc_on_data(const void *data, const void *lut, uint64_t cnt);
extern uint64_t calc_on_data_prefetch1(const void *data, const void *lut, uint64_t cnt);
extern uint64_t calc_on_data_prefetch2(const void *data, const void *lut, uint64_t cnt);
extern uint64_t calc_on_data_prefetch4(const void *data, const void *lut, uint64_t cnt);
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
    if (argc < 2) {
        fprintf(stderr, "Usage: <program> [float count] (csv)\n");
        return 1;
    }

    size_t const count = atol(argv[1]);
    bool const write_csv = argc > 3 && streq(argv[3], "csv");

    size_t const byte_count = count * sizeof(float) * 16;
    size_t const lut_byte_count = count * sizeof(void *);

    init_os_process_state(g_os_proc_state);
    uint64_t cpu_timer_freq = measure_cpu_timer_freq(0.1l);

    srand(time(nullptr));

    RepetitionTester rt{byte_count, cpu_timer_freq, 10.f, false};

    float *mem = (float *)allocate_os_pages_memory(byte_count);
    float **lut = (float **)allocate_os_pages_memory(lut_byte_count);
    if (!mem) {
        fprintf(stderr, "Allocation failed\n");
        return 2;
    }

    for (size_t i = 0; i < byte_count / sizeof(float); ++i)
        mem[i] = float(rand()) / float(RAND_MAX);
    for (size_t i = 0; i < count; ++i)
        lut[i] = &mem[i];

    for (size_t i = 0; i < count - 1; ++i) {
        float *tmp = lut[i];
        size_t swi = rand() % (count - i - 1) + i + 1;
        lut[i] = lut[swi];
        lut[swi] = tmp;
    }

    if (write_csv)
        printf("name,bytes,seconds,gb/s\n");

    run_test(
        [&] { return calc_on_data(mem, lut, count); },
        byte_count, rt, "plain compute", cpu_timer_freq, write_csv);
    run_test(
        [&] { return calc_on_data_prefetch1(mem, lut, count); },
        byte_count, rt, "prefetch 1", cpu_timer_freq, write_csv);
    run_test(
        [&] { return calc_on_data_prefetch2(mem, lut, count); },
        byte_count, rt, "prefetch 2", cpu_timer_freq, write_csv);
    run_test(
        [&] { return calc_on_data_prefetch4(mem, lut, count); },
        byte_count, rt, "prefetch 4", cpu_timer_freq, write_csv);

    free_os_pages_memory(mem, byte_count);
    free_os_pages_memory(lut, lut_byte_count);
    return 0;
}



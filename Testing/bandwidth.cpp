#include <repetition.hpp>
#include <profiling.hpp>
#include <memory.hpp>
#include <os.hpp>
#include <util.hpp>

#include <cstdio>
#include <cstdlib>
#include <cstdint>

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: <program> [byte count]\n");
        return 1;
    }

    const size_t byte_count = atol(argv[1]);

    init_os_process_state(g_os_proc_state);
    uint64_t cpu_timer_freq = measure_cpu_timer_freq(0.1l);

    RepetitionTester rt{"Mem write loop", byte_count, cpu_timer_freq, 10.f, false, true};

    char *mem = (char *)allocate_os_pages_memory(byte_count);
    if (!mem) {
        fprintf(stderr, "Allocation failed\n");
        return 2;
    }

    rt.ReStart();
    do {
        rt.BeginTimeBlock();
        for (size_t i = 0; i < byte_count; ++i)
            mem[i] = char(i);
        rt.EndTimeBlock();

        rt.ReportProcessedBytes(byte_count);
    } while (rt.Tick());

    free_os_pages_memory(mem, byte_count);
    return 0;
}

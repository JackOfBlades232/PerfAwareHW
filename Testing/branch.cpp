#include <repetition.hpp>
#include <profiling.hpp>
#include <memory.hpp>
#include <os.hpp>
#include <util.hpp>

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <cassert>

#if _WIN32
#pragma comment(lib, "Bcrypt.lib")
#endif

extern "C"
{
extern uint64_t run_cond_loop(uint64_t count, const char *ptr);
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
    } while (rt.Tick());
}

void fill_zeroes(char *mem, uint64_t cnt)
{
    memset(mem, 0, cnt);
}

void fill_ones(char *mem, uint64_t cnt)
{
    memset(mem, 1, cnt);
}

void fill_fours(char *mem, uint64_t cnt)
{
    memset(mem, 4, cnt);
}

void fill_every_second(char *mem, uint64_t cnt)
{
    for (char *p = mem; p != mem + cnt; ++p)
        *p = ((p - mem) & 1) ? 1 : 4;
}

void fill_every_third(char *mem, uint64_t cnt)
{
    for (char *p = mem; p != mem + cnt; ++p)
        *p = ((p - mem) % 3 == 0) ? 4 : 1;
}

void fill_every_fourth(char *mem, uint64_t cnt)
{
    for (char *p = mem; p != mem + cnt; ++p)
        *p = ((p - mem) & 3) ? 1 : 4;
}

void fill_every_eighth(char *mem, uint64_t cnt)
{
    for (char *p = mem; p != mem + cnt; ++p)
        *p = ((p - mem) & 7) ? 1 : 4;
}

void fill_c_random(char *mem, uint64_t cnt)
{
    for (char *p = mem; p != mem + cnt; ++p)
        *p = unsigned(rand()) & 0xFF;
}

void fill_os_random(char *mem, uint64_t cnt)
{
#if _WIN32
    NTSTATUS st = BCryptGenRandom(
        nullptr, PUCHAR(mem), cnt, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    assert(st == 0);
#else
    FILE *rf = fopen("/dev/urandom", "rb");
    assert(rf);
    int read = fread(mem, cnt, 1, rf);
    assert(read == 1);
    fclose(rf);
#endif
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: <program> [byte count]\n");
        return 1;
    }

    const size_t byte_count = atol(argv[1]);

    init_os_process_state(g_os_proc_state);
    uint64_t cpu_timer_freq = measure_cpu_timer_freq(0.1l);

    srand(time(nullptr));

    RepetitionTester rt{"Mem write loop", byte_count, cpu_timer_freq, 10.f, false, true};

    char *mem = (char *)allocate_os_pages_memory(byte_count);
    if (!mem) {
        fprintf(stderr, "Allocation failed\n");
        return 2;
    }

#define RUN_TEST(func_name_, memfill_func_name_)      \
    do {                                              \
        memfill_func_name_(mem, byte_count);          \
        run_test([mem, byte_count] {                  \
            return func_name_(byte_count, mem);       \
        }, rt, #func_name_ "__" #memfill_func_name_); \
    } while (0)

    for (;;) {
        RUN_TEST(run_cond_loop, fill_zeroes);
        RUN_TEST(run_cond_loop, fill_ones);
        RUN_TEST(run_cond_loop, fill_fours);
        RUN_TEST(run_cond_loop, fill_every_second);
        RUN_TEST(run_cond_loop, fill_every_third);
        RUN_TEST(run_cond_loop, fill_every_fourth);
        RUN_TEST(run_cond_loop, fill_every_eighth);
        RUN_TEST(run_cond_loop, fill_c_random);
        RUN_TEST(run_cond_loop, fill_os_random);
    }

    free_os_pages_memory(mem, byte_count);
    return 0;
}

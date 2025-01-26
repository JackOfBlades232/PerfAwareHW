#pragma once

#include "util.hpp"

#include <cstdint>

#if _WIN32

#include <intrin.h>
#include <windows.h>

inline uint64_t get_os_timer_freq()
{
    LARGE_INTEGER freq;
    BOOL res = QueryPerformanceFrequency(&freq);
    if (!res)
        return uint64_t(-1);
    return freq.QuadPart;
}

inline uint64_t read_os_timer()
{
    LARGE_INTEGER ticks;
    BOOL res = QueryPerformanceCounter(&ticks);
    if (!res)
        return uint64_t(-1);
    return ticks.QuadPart;
}

#else

#include <x86intrin.h>
#include <ctime>

inline uint64_t get_os_timer_freq()
{
    return 1'000'000'000;
}

inline uint64_t read_os_timer()
{
    struct timespec ts;
    int res = clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    if (res != 0)
        return uint64_t(-1);
    return ts.tv_sec * 1'000'000'000 + ts.tv_nsec;
}

#endif

inline uint64_t read_cpu_timer()
{
    return __rdtsc();
}

inline uint64_t measure_cpu_timer_freq(long double measure_time_sec)
{
    uint64_t os_freq = get_os_timer_freq();
    uint64_t measure_ticks = (uint64_t)((long double)os_freq * measure_time_sec);

    uint64_t start_cpu_tick = read_cpu_timer();
    uint64_t start_os_tick = read_os_timer();

    while (read_os_timer() - start_os_tick < measure_ticks)
        ;

    uint64_t end_cpu_ticks = read_cpu_timer();

    return (uint64_t)((long double)(end_cpu_ticks - start_cpu_tick) / measure_time_sec);
}

inline long double get_cpu_sec_from(uint64_t ref_ticks, uint64_t freq)
{
    uint64_t delta_ticks = read_cpu_timer() - ref_ticks;
    return (long double)delta_ticks / freq;
}

// @TODO: aggregate per function with hash table, in finalization

struct profiler_measurement_t {
    long double seconds;
    const char *name;
    uint32_t id_in_name;
};

inline profiler_measurement_t g_profiler_measurements[10'000] = {};
inline uint32_t g_profiler_measurements_cnt = 0;
inline uint64_t g_cpu_timer_freq = 0;
inline uint64_t g_start_ticks = 0;

inline void init_profiler()
{
    g_cpu_timer_freq = measure_cpu_timer_freq(0.1l);
    g_start_ticks = read_cpu_timer();
}

template <class TPrinter>
inline void finish_profiling_and_dump_stats(TPrinter &&printer)
{
    // @TODO: less dumb recollection of ids, hash map would help
    long double total_seconds = get_cpu_sec_from(g_start_ticks, g_cpu_timer_freq);

    printer("Profile:\n");
    for (uint32_t i = 0; i < g_profiler_measurements_cnt; ++i) {
        auto &measurement = g_profiler_measurements[i];
        for (uint32_t j = 0; j < i; ++j) {
            const auto &other = g_profiler_measurements[j];
            if (streq(measurement.name, other.name) && other.id_in_name >= measurement.id_in_name)
                measurement.id_in_name = other.id_in_name + 1;
        }

        printer("%s[%u]: %Lfs (%.1Lf%%)\n",
                measurement.name, measurement.id_in_name,
                measurement.seconds, 100.l * measurement.seconds / total_seconds);
    }
    printer("Total: %Lfs\n", total_seconds);
}

class ScopedProfile {
    uint64_t m_ref_ticks;
    const char *m_name;

public:
    ScopedProfile(const char *name) : m_name{name} { m_ref_ticks = read_cpu_timer(); }
    ~ScopedProfile() {
        long double seconds = get_cpu_sec_from(m_ref_ticks, g_cpu_timer_freq);
        g_profiler_measurements[g_profiler_measurements_cnt++] = {seconds, m_name, 0};
    }

    ScopedProfile(const ScopedProfile &) = delete;
    ScopedProfile& operator=(const ScopedProfile &) = delete;
    ScopedProfile(ScopedProfile &&) = delete;
    ScopedProfile& operator=(ScopedProfile &&) = delete;
};

#define PROFILED_BLOCK(name_) ScopedProfile CAT(profiled_block__, __COUNTER__){name_}
#define PROFILED_FUNCTION PROFILED_BLOCK(__FUNCTION__)

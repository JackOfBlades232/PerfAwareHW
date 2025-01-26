#pragma once

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

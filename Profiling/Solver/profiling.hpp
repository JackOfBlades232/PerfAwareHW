#pragma once

#include "util.hpp"
#include "algo.hpp"

#include <cassert>
#include <cfloat>
#include <cstddef>
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

#ifndef READ_TIMER
#define READ_TIMER read_cpu_timer
#endif

inline uint64_t measure_cpu_timer_freq(long double measure_time_sec)
{
    uint64_t os_freq = get_os_timer_freq();
    uint64_t measure_ticks = (uint64_t)((long double)os_freq * measure_time_sec);

    uint64_t start_cpu_tick = READ_TIMER();
    uint64_t start_os_tick = read_os_timer();

    while (read_os_timer() - start_os_tick < measure_ticks)
        ;

    uint64_t end_cpu_ticks = READ_TIMER();

    return (uint64_t)((long double)(end_cpu_ticks - start_cpu_tick) / measure_time_sec);
}

inline long double ticks_to_sec(uint64_t ticks, uint64_t freq)
{
    return (long double)ticks / freq;
}

struct profiler_slot_t {
    uint64_t inclusive_ticks = 0;
    uint64_t exclusive_ticks = 0;
    const char *name = nullptr;
    uint32_t hit_count = 0;
};

inline struct profiler_t {
    profiler_slot_t slots[4096] = {};
    uint64_t start_ticks = 0;
    uint64_t end_ticks = 0;
    profiler_slot_t *current_slot = &slots[0];
} g_profiler{};

inline constexpr size_t c_profiler_slots_count =
    sizeof(g_profiler.slots) / sizeof(g_profiler.slots[0]);

inline void init_profiler()
{
    g_profiler.start_ticks = READ_TIMER();
}

template <class TPrinter>
inline void finish_profiling_and_dump_stats(TPrinter &&printer)
{
    g_profiler.end_ticks = READ_TIMER();

    const uint64_t cpu_timer_freq = measure_cpu_timer_freq(0.1l);
    const long double total_sec =
        ticks_to_sec(g_profiler.end_ticks - g_profiler.start_ticks, cpu_timer_freq);

    insertion_sort(&g_profiler.slots[0],
                   &g_profiler.slots[c_profiler_slots_count],
                   [](const profiler_slot_t &s1, const profiler_slot_t &s2) {
                       // Decreasing order with empty slots pushed to end
                       return s1.name && (!s2.name || s1.inclusive_ticks > s2.inclusive_ticks);
                   });

    printer("Profile:\n");
    for (size_t i = 0; i < c_profiler_slots_count; ++i) {
        const auto &slot = g_profiler.slots[i];

        if (!slot.name)
            continue;

        const long double inclusive_sec = ticks_to_sec(slot.inclusive_ticks, cpu_timer_freq);
        const long double exclusive_sec = ticks_to_sec(slot.exclusive_ticks, cpu_timer_freq);

        if (abs(inclusive_sec - exclusive_sec) < LDBL_EPSILON) {
            printer("%s[%u]: %Lfs (%.1Lf%%)\n",
                    slot.name, slot.hit_count,
                    inclusive_sec, 100.l * inclusive_sec / total_sec);
        } else {
            printer("%s[%u]: %Lfs (%.1Lf%%) inclusive, %Lfs (%.1Lf%%) exclusive\n",
                    slot.name, slot.hit_count,
                    inclusive_sec, 100.l * inclusive_sec / total_sec,
                    exclusive_sec, 100.l * exclusive_sec / total_sec);
        }
    }
    printer("Total: %Lfs\n", total_sec);
}

template <uint32_t t_slot>
class ScopedProfile {
    uint64_t m_ref_ticks;
    uint64_t m_inclusive_snapshot;
    profiler_slot_t *m_parent;

public:
    ScopedProfile(const char *name) {
        auto &slot = g_profiler.slots[t_slot];

        slot.name = name;
        ++slot.hit_count;
        m_parent = xchg(g_profiler.current_slot, &g_profiler.slots[t_slot]);

        m_inclusive_snapshot = slot.inclusive_ticks;
        m_ref_ticks = READ_TIMER();
    }
    ~ScopedProfile() {
        uint64_t delta_ticks = READ_TIMER() - m_ref_ticks;

        auto &slot = g_profiler.slots[t_slot];

        slot.exclusive_ticks += delta_ticks;
        m_parent->exclusive_ticks -= delta_ticks;

        slot.inclusive_ticks = m_inclusive_snapshot + delta_ticks;

        g_profiler.current_slot = m_parent;
    }

    ScopedProfile(const ScopedProfile &) = delete;
    ScopedProfile& operator=(const ScopedProfile &) = delete;
    ScopedProfile(ScopedProfile &&) = delete;
    ScopedProfile& operator=(ScopedProfile &&) = delete;
};

// @TODO: figure out a cross-translation unit decision for fast indexing

#if PROFILER

#define PROFILED_BLOCK(name_) ScopedProfile<__COUNTER__ + 1> CAT(profiled_block__, __LINE__){name_}
#define PROFILED_FUNCTION PROFILED_BLOCK(__FUNCTION__)

#else

#define PROFILED_BLOCK(name_)
#define PROFILED_FUNCTION

#endif

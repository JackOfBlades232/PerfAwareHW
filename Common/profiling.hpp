#pragma once

#include "util.hpp"
#include "os.hpp"
#include "algo.hpp"

#include <cassert>
#include <cfloat>
#include <cstddef>
#include <cstdint>

#if _WIN32

#include <intrin.h>
#include <windows.h>
#include <psapi.h>

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

inline uint64_t read_process_page_faults()
{
    // @TODO: this should always be enough, right?
    PROCESS_MEMORY_COUNTERS_EX memory_counters = {};
    memory_counters.cb = sizeof(memory_counters);
    BOOL res = GetProcessMemoryInfo(g_os_proc_state.process_hnd,
                                    (PROCESS_MEMORY_COUNTERS *)&memory_counters,
                                    sizeof(memory_counters));
    if (!res)
        return uint64_t(-1);
    return uint64_t(memory_counters.PageFaultCount);
}

#else

#include <x86intrin.h>
#include <ctime>
#include <cstdio>

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

inline uint64_t read_process_page_faults()
{
    FILE *procdata_f = fopen(g_os_proc_state.stat_file_name_buf, "r");
    if (!procdata_f)
        return uint64_t(-1);
    unsigned long minor_fault_count;

    {
        char csink;
        int isink;
        unsigned usink;
        char ssink[32]; // enough by man
        int items = fscanf(procdata_f, "%d %s %c %d %d %d %d %d %u %lu", &isink,
                           ssink, &csink, &isink, &isink, &isink, &isink, &isink,
                           &usink, &minor_fault_count);
        if (items != 10)
            minor_fault_count = (unsigned long)(-1);
    }
    fclose(procdata_f);

    return minor_fault_count == (unsigned long)(-1)
               ? uint64_t(-1)
               : uint64_t(minor_fault_count);
}

#endif

inline uint64_t read_cpu_timer()
{
    return __rdtsc();
}

#ifndef READ_TIMER
#define READ_TIMER read_cpu_timer
#endif
#ifndef READ_PAGE_FAULT_COUNTER
#define READ_PAGE_FAULT_COUNTER read_process_page_faults
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
#if PROFILER_PAGEFAULTS
    uint64_t inclusive_pagefaults = 0;
    uint64_t exclusive_pagefaults = 0;
#endif
    uint64_t bytes_processed = 0;
    const char *name = nullptr;
    uint32_t hit_count = 0;
};

inline struct profiler_t {
    profiler_slot_t slots[4096] = {};
    uint64_t start_ticks = 0;
    uint64_t end_ticks = 0;
#if PROFILER_PAGEFAULTS
    uint64_t start_pagefaults = 0;
    uint64_t end_pagefaults = 0;
#endif
    profiler_slot_t *current_slot = &slots[0];
} g_profiler{};

inline constexpr size_t c_profiler_slots_count =
    sizeof(g_profiler.slots) / sizeof(g_profiler.slots[0]);

inline void init_profiler()
{
#if PROFILER_PAGEFAULTS
    g_profiler.start_pagefaults = READ_PAGE_FAULT_COUNTER();
#endif
    g_profiler.start_ticks = READ_TIMER();
}

template <class TPrinter>
inline void finish_profiling_and_dump_stats(TPrinter &&printer)
{
    g_profiler.end_ticks = READ_TIMER();
#if PROFILER_PAGEFAULTS
    g_profiler.end_pagefaults = READ_PAGE_FAULT_COUNTER();
#endif

    const uint64_t cpu_timer_freq = measure_cpu_timer_freq(0.1l);
    const long double total_sec = ticks_to_sec(
        g_profiler.end_ticks - g_profiler.start_ticks, cpu_timer_freq);

    insertion_sort(
        &g_profiler.slots[0],
        &g_profiler.slots[c_profiler_slots_count],
        [](const profiler_slot_t &s1, const profiler_slot_t &s2) {
            // Decreasing order with empty slots pushed to end
            return
                s1.name &&
                (!s2.name || s1.inclusive_ticks > s2.inclusive_ticks);
        });

    printer("Profile:\n");
    for (size_t i = 0; i < c_profiler_slots_count; ++i) {
        const auto &slot = g_profiler.slots[i];

        if (!slot.name)
            continue;

        const long double inclusive_sec =
            ticks_to_sec(slot.inclusive_ticks, cpu_timer_freq);
        const long double exclusive_sec =
            ticks_to_sec(slot.exclusive_ticks, cpu_timer_freq);

        if (abs(inclusive_sec - exclusive_sec) < LDBL_EPSILON) {
            printer("%s[%u]: %Lfs (%.1Lf%%)",
                slot.name, slot.hit_count,
                inclusive_sec, 100.l * inclusive_sec / total_sec);
        } else {
            printer("%s[%u]: %Lfs (%.1Lf%%) inc, %Lfs (%.1Lf%%) exc",
                slot.name, slot.hit_count,
                inclusive_sec, 100.l * inclusive_sec / total_sec,
                exclusive_sec, 100.l * exclusive_sec / total_sec);
        }

        if (slot.bytes_processed > 0) {
            constexpr long double c_bytes_in_mb = (long double)(1u << 20);
            constexpr long double c_bytes_in_gb = (long double)(1u << 30);
            const long double mb =
                (long double)slot.bytes_processed / c_bytes_in_mb; 
            const long double gb =
                (long double)slot.bytes_processed / c_bytes_in_gb; 
            const long double gb_per_sec = gb / inclusive_sec;

            printer(", %.3Lfmb (%.2Lfgb/s)", mb, gb_per_sec);
        }

#if PROFILER_PAGEFAULTS
        uint64_t const total_pagefaults =
            g_profiler.end_pagefaults - g_profiler.start_pagefaults;
        if (slot.inclusive_pagefaults == 0) {
            // skip
        } else if (slot.inclusive_pagefaults == slot.exclusive_pagefaults) {
            printer(", %llu pfaults (%.1Lf%%)",
                slot.inclusive_pagefaults,
                100.l * slot.inclusive_pagefaults / total_pagefaults);
        } else {
            printer(", %llu pfaults inc (%.1Lf%%), %llu pfaults exc (%.1Lf%%)",
                slot.inclusive_pagefaults,slot.exclusive_pagefaults,
                100.l * slot.inclusive_pagefaults / total_pagefaults,
                100.l * slot.exclusive_pagefaults / total_pagefaults);
        }
#endif

        printer("\n");
    }
    printer("Total: %Lfs\n", total_sec);
}

template <uint32_t t_slot, bool t_pagefaults>
class ScopedProfile {
    uint64_t m_ref_ticks;
    uint64_t m_inclusive_snapshot;
#if PROFILER_PAGEFAULTS
    uint64_t m_ref_pagefaults;
    uint64_t m_inclusive_pf_snapshot;
#endif
    profiler_slot_t *m_parent;

public:
    ScopedProfile(const char *name, uint64_t bytes = 0) {
        auto &slot = g_profiler.slots[t_slot];

        slot.name = name;
        ++slot.hit_count;
        slot.bytes_processed += bytes;
        m_parent = xchg(g_profiler.current_slot, &g_profiler.slots[t_slot]);

#if PROFILER_PAGEFAULTS
        if constexpr (t_pagefaults) {
            m_inclusive_pf_snapshot = slot.inclusive_pagefaults;
            m_ref_pagefaults = READ_PAGE_FAULT_COUNTER();
        }
#endif
        m_inclusive_snapshot = slot.inclusive_ticks;
        m_ref_ticks = READ_TIMER();
    }
    ~ScopedProfile() {
        uint64_t delta_ticks = READ_TIMER() - m_ref_ticks;
#if PROFILER_PAGEFAULTS
        uint64_t delta_pagefaults = 0;
        if constexpr (t_pagefaults) {
            delta_pagefaults =
                READ_PAGE_FAULT_COUNTER() - m_ref_pagefaults;
        }
#endif

        auto &slot = g_profiler.slots[t_slot];

        slot.exclusive_ticks += delta_ticks;
        m_parent->exclusive_ticks -= delta_ticks;

        slot.inclusive_ticks = m_inclusive_snapshot + delta_ticks;

#if PROFILER_PAGEFAULTS
        if constexpr (t_pagefaults) {
            slot.exclusive_pagefaults += delta_pagefaults;
            m_parent->exclusive_pagefaults -= delta_pagefaults;
            slot.inclusive_pagefaults =
                m_inclusive_pf_snapshot + delta_pagefaults;
        }
#endif

        g_profiler.current_slot = m_parent;
    }

    ScopedProfile(const ScopedProfile &) = delete;
    ScopedProfile& operator=(const ScopedProfile &) = delete;
    ScopedProfile(ScopedProfile &&) = delete;
    ScopedProfile& operator=(ScopedProfile &&) = delete;
};

// @TODO: figure out a cross-translation unit decision for fast indexing

#if PROFILER

#define PROFILED_BLOCK(name_)             \
    ScopedProfile<__COUNTER__ + 1, false> \
    CAT(profiled_block__, __LINE__){name_}
#define PROFILED_FUNCTION PROFILED_BLOCK(__FUNCTION__)

#define PROFILED_BANDWIDTH_BLOCK(name_, bytes_) \
    ScopedProfile<__COUNTER__ + 1, false>       \
    CAT(profiled_block__, __LINE__){name_, bytes_}
#define PROFILED_BANDWIDTH_FUNCTION(bytes_) \
    PROFILED_BANDWIDTH_BLOCK(__FUNCTION__, bytes_)

#define PROFILED_BLOCK_PF(name_)         \
    ScopedProfile<__COUNTER__ + 1, true> \
    CAT(profiled_block__, __LINE__){name_}
#define PROFILED_FUNCTION_PF PROFILED_BLOCK_PF(__FUNCTION__)

#define PROFILED_BANDWIDTH_BLOCK_PF(name_, bytes_) \
    ScopedProfile<__COUNTER__ + 1, true>           \
    CAT(profiled_block__, __LINE__){name_, bytes_}
#define PROFILED_BANDWIDTH_FUNCTION_PF(bytes_) \
    PROFILED_BANDWIDTH_BLOCK_PF(__FUNCTION__, bytes_)

#else

#define PROFILED_BLOCK(name_)
#define PROFILED_FUNCTION
#define PROFILED_BANDWIDTH_BLOCK(name_, bytes_)
#define PROFILED_BANDWIDTH_FUNCTION(bytes_)

#define PROFILED_BLOCK_PF(name_)
#define PROFILED_FUNCTION_PF
#define PROFILED_BANDWIDTH_BLOCK_PF(name_, bytes_)
#define PROFILED_BANDWIDTH_FUNCTION_PF(bytes_)

#endif

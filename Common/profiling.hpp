#pragma once

#include "defs.hpp"
#include "os.hpp"
#include "algo.hpp"

#if _WIN32

#include <intrin.h>
#include <windows.h>
#include <psapi.h>

inline u64 get_os_timer_freq()
{
    LARGE_INTEGER freq;
    BOOL res = QueryPerformanceFrequency(&freq);
    if (!res)
        return u64(-1);
    return freq.QuadPart;
}

inline u64 read_os_timer()
{
    LARGE_INTEGER ticks;
    BOOL res = QueryPerformanceCounter(&ticks);
    if (!res)
        return u64(-1);
    return ticks.QuadPart;
}

inline u64 read_process_page_faults()
{
    // @TODO: this should always be enough, right?
    PROCESS_MEMORY_COUNTERS_EX memory_counters = {};
    memory_counters.cb = sizeof(memory_counters);
    BOOL res = GetProcessMemoryInfo(
        g_os_proc_state.process_hnd,
        (PROCESS_MEMORY_COUNTERS *)&memory_counters,
        sizeof(memory_counters));
    if (!res)
        return u64(-1);
    return u64(memory_counters.PageFaultCount);
}

#else

#include <x86intrin.h>

inline u64 get_os_timer_freq()
{
    return 1'000'000'000;
}

inline u64 read_os_timer()
{
    struct timespec ts;
    int res = clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    if (res != 0)
        return u64(-1);
    return ts.tv_sec * 1'000'000'000 + ts.tv_nsec;
}

inline u64 read_process_page_faults()
{
    FILE *procdata_f = fopen(g_os_proc_state.stat_file_name_buf, "r");
    if (!procdata_f)
        return u64(-1);
    ulong minor_fault_count;

    {
        char csink;
        int isink;
        uint usink;
        char ssink[32]; // enough by man
        int items = fscanf(
            procdata_f, "%d %s %c %d %d %d %d %d %u %lu", &isink,
            ssink, &csink, &isink, &isink, &isink, &isink, &isink,
            &usink, &minor_fault_count);
        if (items != 10)
            minor_fault_count = ulong(-1);
    }
    fclose(procdata_f);

    return minor_fault_count == ulong(-1) ? u64(-1) : u64(minor_fault_count);
}

#endif

inline u64 read_cpu_timer()
{
    return __rdtsc();
}

#ifndef READ_TIMER
#define READ_TIMER read_cpu_timer
#endif
#ifndef READ_PAGE_FAULT_COUNTER
#define READ_PAGE_FAULT_COUNTER read_process_page_faults
#endif

inline u64 measure_cpu_timer_freq(f64 measure_time_sec)
{
    u64 os_freq = get_os_timer_freq();
    u64 measure_ticks = u64(f64(os_freq) * measure_time_sec);

    u64 start_cpu_tick = READ_TIMER();
    u64 start_os_tick = read_os_timer();

    while (read_os_timer() - start_os_tick < measure_ticks)
        ;

    u64 end_cpu_ticks = READ_TIMER();

    return u64(f64(end_cpu_ticks - start_cpu_tick) / measure_time_sec);
}

inline f64 ticks_to_sec(u64 ticks, u64 freq)
{
    return large_divide(ticks, freq);
}

struct profiler_slot_t {
    u64 inclusive_ticks = 0;
    u64 exclusive_ticks = 0;
#if PROFILER_PAGEFAULTS
    u64 inclusive_pagefaults = 0;
    u64 exclusive_pagefaults = 0;
#endif
    u64 bytes_processed = 0;
    char const *name = nullptr;
    u32 hit_count = 0;
};

inline struct profiler_t {
    profiler_slot_t slots[4096] = {};
    u64 start_ticks = 0;
    u64 end_ticks = 0;
#if PROFILER_PAGEFAULTS
    u64 start_pagefaults = 0;
    u64 end_pagefaults = 0;
#endif
    profiler_slot_t *current_slot = &slots[0];
} g_profiler{};

inline constexpr usize c_profiler_slots_count =
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

    u64 const cpu_timer_freq = measure_cpu_timer_freq(0.1l);
    f64 const total_sec = ticks_to_sec(
        g_profiler.end_ticks - g_profiler.start_ticks, cpu_timer_freq);

    insertion_sort(
        &g_profiler.slots[0],
        &g_profiler.slots[c_profiler_slots_count],
        [](profiler_slot_t const &s1, profiler_slot_t const &s2) {
            // Decreasing order with empty slots pushed to end
            return
                s1.name &&
                (!s2.name || s1.inclusive_ticks > s2.inclusive_ticks);
        });

    printer("Profile:\n");
    for (usize i = 0; i < c_profiler_slots_count; ++i) {
        auto const &slot = g_profiler.slots[i];

        if (!slot.name)
            continue;

        f64 const inclusive_sec =
            ticks_to_sec(slot.inclusive_ticks, cpu_timer_freq);
        f64 const exclusive_sec =
            ticks_to_sec(slot.exclusive_ticks, cpu_timer_freq);

        if (abs(inclusive_sec - exclusive_sec) < DBL_EPSILON) {
            printer("%s[%u]: %lfs (%.1lf%%)",
                slot.name, slot.hit_count,
                inclusive_sec, 100.0 * inclusive_sec / total_sec);
        } else {
            printer("%s[%u]: %lfs (%.1lf%%) inc, %lfs (%.1lf%%) exc",
                slot.name, slot.hit_count,
                inclusive_sec, 100.0 * inclusive_sec / total_sec,
                exclusive_sec, 100.0 * exclusive_sec / total_sec);
        }

        if (slot.bytes_processed > 0) {
            constexpr u64 c_bytes_in_mb = 1u << 20;
            constexpr u64 c_bytes_in_gb = 1u << 30;
            f64 const mb = large_divide(slot.bytes_processed, c_bytes_in_mb); 
            f64 const gb = large_divide(slot.bytes_processed, c_bytes_in_gb); 
            f64 const gb_per_sec = gb / inclusive_sec;

            printer(", %.3lfmb (%.2lfgb/s)", mb, gb_per_sec);
        }

#if PROFILER_PAGEFAULTS
        u64 const total_pagefaults =
            g_profiler.end_pagefaults - g_profiler.start_pagefaults;
        if (slot.inclusive_pagefaults == 0) {
        } else if (slot.inclusive_pagefaults == slot.exclusive_pagefaults) {
            printer(", %llu pfaults (%.1lf%%)",
                slot.inclusive_pagefaults,
                100.0 * slot.inclusive_pagefaults / total_pagefaults);
        } else {
            printer(", %llu pfaults inc (%.1lf%%), %llu pfaults exc (%.1lf%%)",
                slot.inclusive_pagefaults,slot.exclusive_pagefaults,
                100.0 * slot.inclusive_pagefaults / total_pagefaults,
                100.0 * slot.exclusive_pagefaults / total_pagefaults);
        }
#endif

        printer("\n");
    }
    printer("Total: %lfs\n", total_sec);
}

template <u32 t_slot, bool t_pagefaults>
class ScopedProfile {
    u64 m_ref_ticks;
    u64 m_inclusive_snapshot;
#if PROFILER_PAGEFAULTS
    u64 m_ref_pagefaults;
    u64 m_inclusive_pf_snapshot;
#endif
    profiler_slot_t *m_parent;

public:
    ScopedProfile(char const *name, u64 bytes = 0) {
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
        u64 delta_ticks = READ_TIMER() - m_ref_ticks;
#if PROFILER_PAGEFAULTS
        u64 delta_pagefaults = 0;
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

    ScopedProfile(ScopedProfile const &) = delete;
    ScopedProfile& operator=(ScopedProfile const &) = delete;
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

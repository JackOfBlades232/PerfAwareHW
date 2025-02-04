#pragma once

#include "profiling.hpp"

// @TODO customizable printer instead?
#include <cstdio>

class RepetitionTester {
    enum State {
        e_st_pending,
        e_st_active,
        e_st_error
    } m_state;

    uint64_t m_target_processed_bytes;
    uint64_t m_cpu_timer_freq;
    uint64_t m_try_renew_min_for_ticks;
    uint64_t m_test_start_ticks;

    uint64_t m_ticks;
    uint64_t m_bytes;

    uint32_t m_open_blocks;
    uint32_t m_closed_blocks;

    bool m_print_new_minimums;

    uint64_t m_test_count;
    uint64_t m_total_ticks;
    uint64_t m_min_ticks;
    uint64_t m_max_ticks;

    const char *m_name;

#define RT_ERR(text_, ...)                           \
    do {                                             \
        printf("Error: " text_ "\n", ##__VA_ARGS__); \
        m_state = e_st_error;                        \
        return;                                      \
    } while (0) 
#define RT_ERRET(ret_, text_, ...)                   \
    do {                                             \
        printf("Error: " text_ "\n", ##__VA_ARGS__); \
        m_state = e_st_error;                        \
        return ret_;                                 \
    } while (0) 
#define RT_PRINT(text_, ...) printf(text_ "\n", ##__VA_ARGS__)

public:
    RepetitionTester(const char *name,
                     uint64_t target_bytes,
                     uint64_t cpu_timer_freq,
                     float seconds_for_min_renewal,
                     bool print_minimums)
        : m_state{e_st_pending}
        , m_target_processed_bytes{target_bytes}
        , m_cpu_timer_freq{cpu_timer_freq}
        , m_try_renew_min_for_ticks{uint64_t(seconds_for_min_renewal * cpu_timer_freq)}
        , m_print_new_minimums{print_minimums}
        , m_name{name} {}

    void ReStart() {
        if (m_state != e_st_pending)
            RT_ERR("Start called on non-pending rep tester");
        m_test_count = 0;
        m_total_ticks = 0;
        m_min_ticks = uint64_t(-1);
        m_max_ticks = 0;
        m_ticks = 0;
        m_bytes = 0;
        m_open_blocks = 0;
        m_closed_blocks = 0;
        m_state = e_st_active;
        m_test_start_ticks = READ_TIMER();
    }

    bool Tick() {
        if (m_state == e_st_pending)
            RT_ERRET(false, "Finish called on non-active rep tester");
        else if (m_state == e_st_error)
            return false;

        uint64_t cur_ticks = READ_TIMER();
        if (m_open_blocks) {
            if (m_open_blocks != m_closed_blocks)
                RT_ERRET(false, "Not all timed blocks closed in rep tester");
            if (m_bytes != m_target_processed_bytes)
                RT_ERRET(false, "Bytes processed in test not equal to target bytes");

            ++m_test_count;
            m_total_ticks += m_ticks;
            if (m_max_ticks < m_ticks)
                m_max_ticks = m_ticks;
            if (m_min_ticks > m_ticks) {
                m_min_ticks = m_ticks;
                m_test_start_ticks = cur_ticks;
                if (m_print_new_minimums)
                    RT_PRINT("Found new min time: %Lfs", (long double)m_min_ticks / m_cpu_timer_freq);
            }
        }

        m_ticks = 0;
        m_bytes = 0;
        m_open_blocks = 0;
        m_closed_blocks = 0;

        if (cur_ticks - m_test_start_ticks > m_try_renew_min_for_ticks) {
            FinishAndPrintResults();
            return false;
        }

        return true;
    }

    void BeginTimeBlock() {
        ++m_open_blocks;
        m_ticks -= READ_TIMER();
    }

    void EndTimeBlock() {
        ++m_closed_blocks;
        m_ticks += READ_TIMER();
    }

    void ReportProcessedBytes(uint64_t bytes) { m_bytes += bytes; }

    // @TODO format?
    void ReportError(const char *text) { RT_ERR("%s", text); }

private:
    void FinishAndPrintResults() {
        m_state = e_st_pending;

        // @TODO: pull out these computations
        auto gb_per_sec = [](long double sec, uint64_t bytes) {
            constexpr long double c_bytes_in_gb = (long double)(1u << 30);
            const long double gb = (long double)bytes / c_bytes_in_gb; 
            return gb / sec;
        };
        const long double min_sec = (long double)m_min_ticks / m_cpu_timer_freq;
        const long double max_sec = (long double)m_max_ticks / m_cpu_timer_freq;

        const uint64_t avg_ticks = m_total_ticks / m_test_count;
        const long double avg_sec = ((long double)m_total_ticks / m_test_count) / m_cpu_timer_freq;

        RT_PRINT("");
        RT_PRINT("--- %s ---", m_name);
        RT_PRINT("Min: %Lf (%llu), %Lfgb/s", min_sec, m_min_ticks, gb_per_sec(min_sec, m_target_processed_bytes));
        RT_PRINT("Max: %Lf (%llu), %Lfgb/s", max_sec, m_max_ticks, gb_per_sec(max_sec, m_target_processed_bytes));
        RT_PRINT("Avg: %Lf (%llu), %Lfgb/s", avg_sec, avg_ticks, gb_per_sec(avg_sec, m_target_processed_bytes));
        RT_PRINT("");
    }

#undef RT_ERR
#undef RT_ERRET
#undef RT_PRINT
};

#pragma once

#include "profiling.hpp"
#include "util.hpp"

#if !defined(RT_PRINT) || !defined(RT_PRINTLN)
#include <cstdio>
#define RT_PRINT(text_, ...) printf(text_, ##__VA_ARGS__)
#define RT_PRINTLN(text_, ...) printf(text_ "\n", ##__VA_ARGS__)
#endif

#if !defined(RT_CLEAR) || !defined(RT_CLEARLN)
#include <cstdio>
#define RT_CLEAR(count_)                         \
    do {                                         \
        for (size_t i_ = 0; i_ < (count_); ++i_) \
            putchar('\b');                       \
    } while (0)
#define RT_CLEARLN(count_)                       \
    do {                                         \
        for (size_t i_ = 0; i_ < (count_); ++i_) \
            putchar('\b');                       \
        for (size_t i_ = 0; i_ < (count_); ++i_) \
            putchar(' ');                        \
    } while (0)
#endif

#define RT_PRESERVE_BYTES_TARGET uint64_t(0)

struct repetition_test_results_t {
    uint64_t test_count = 0;
    uint64_t total_bytes = 0;
    uint64_t min_ticks = uint64_t(-1);
    uint64_t max_ticks = 0;
    uint64_t total_ticks = 0;
    uint64_t min_test_page_faults = 0;
    uint64_t max_test_page_faults = 0;
    uint64_t total_page_faults = 0;
};

class RepetitionTester {
    enum state_t {
        e_st_pending,
        e_st_active,
        e_st_error
    } m_state;

    uint64_t m_target_bytes = 0;
    uint64_t m_try_renew_min_for_ticks = 0;
    uint64_t m_test_start_ticks = 0;
    uint64_t m_cpu_timer_freq = 0;

    uint64_t m_ticks = 0;
    uint64_t m_page_faults = 0;
    uint64_t m_bytes = 0;

    uint32_t m_open_blocks = 0;
    uint32_t m_closed_blocks = 0;

    uint32_t m_last_chars_printed_for_min = 0;
    bool m_print_new_minimums = false;

    repetition_test_results_t *m_results = nullptr;

#define RT_ERR(text_, ...)                                    \
    do {                                                      \
        fprintf(stderr, "Error: " text_ "\n", ##__VA_ARGS__); \
        m_state = e_st_error;                                 \
        return;                                               \
    } while (0) 
#define RT_ERRET(ret_, text_, ...)                            \
    do {                                                      \
        fprintf(stderr, "Error: " text_ "\n", ##__VA_ARGS__); \
        m_state = e_st_error;                                 \
        return ret_;                                          \
    } while (0) 

  public:
    RepetitionTester(
            uint64_t target_bytes, uint64_t cpu_timer_freq,
            float seconds_for_min_renewal, bool print_minimums = false)
        : m_state{e_st_pending}
        , m_target_bytes{target_bytes}
        , m_try_renew_min_for_ticks{uint64_t(seconds_for_min_renewal * cpu_timer_freq)}
        , m_cpu_timer_freq{cpu_timer_freq}
        , m_print_new_minimums{print_minimums}
        , m_results{nullptr} {}

    void ReStart(
        repetition_test_results_t &out_results,
        uint64_t new_target_bytes = RT_PRESERVE_BYTES_TARGET)
    {
        if (m_state != e_st_pending)
            RT_ERR("Start called on non-pending rep tester");
        if (new_target_bytes != RT_PRESERVE_BYTES_TARGET)
            m_target_bytes = new_target_bytes;
        m_results = &out_results;
        *m_results = repetition_test_results_t{};
        m_ticks = 0;
        m_page_faults = 0;
        m_bytes = 0;
        m_open_blocks = 0;
        m_closed_blocks = 0;
        m_last_chars_printed_for_min = 0;
        m_state = e_st_active;
        m_test_start_ticks = READ_TIMER();
    }

    bool Tick() {
        if (m_state == e_st_pending)
            RT_ERRET(false, "Finish called on non-active rep tester");
        else if (m_state == e_st_error)
            RT_ERRET(false, "Error during test");

        uint64_t cur_ticks = READ_TIMER();
        if (m_open_blocks) {
            if (m_open_blocks != m_closed_blocks)
                RT_ERRET(false, "Not all timed blocks closed in rep tester");
            if (m_bytes != m_target_bytes)
                RT_ERRET(false, "Bytes processed in test not equal to target bytes");

            ++m_results->test_count;
            m_results->total_ticks += m_ticks;
            m_results->total_bytes += m_bytes;
            m_results->total_page_faults += m_page_faults;
            if (m_results->max_ticks < m_ticks) {
                m_results->max_ticks = m_ticks;
                m_results->max_test_page_faults = m_page_faults;
            }
            if (m_results->min_ticks > m_ticks) {
                m_results->min_ticks = m_ticks;
                m_results->min_test_page_faults = m_page_faults;
                m_test_start_ticks = cur_ticks;
                if (m_print_new_minimums) {
                    RT_CLEAR(m_last_chars_printed_for_min);
                    m_last_chars_printed_for_min = RT_PRINT(
                        "Found new min time: %Lfs",
                        (long double)m_results->min_ticks / m_cpu_timer_freq);
                }
            }
        }

        m_ticks = 0;
        m_bytes = 0;
        m_page_faults = 0;
        m_open_blocks = 0;
        m_closed_blocks = 0;

        if (cur_ticks - m_test_start_ticks > m_try_renew_min_for_ticks) {
            if (m_print_new_minimums)
                RT_CLEARLN(m_last_chars_printed_for_min);
            m_state = e_st_pending;
            return false;
        }

        return true;
    }

    void BeginTimeBlock() {
        ++m_open_blocks;
        m_page_faults -= READ_PAGE_FAULT_COUNTER();
        m_ticks -= READ_TIMER();
    }

    void EndTimeBlock() {
        m_ticks += READ_TIMER();
        m_page_faults += READ_PAGE_FAULT_COUNTER();
        ++m_closed_blocks;
    }

    void ReportProcessedBytes(uint64_t bytes) { m_bytes += bytes; }
    void ReportError(char const *text) { RT_ERR("%s", text); }

    uint64_t GetTargetBytes() const { return m_target_bytes; }

#undef RT_ERR
#undef RT_ERRET
#undef RT_CLEAR
};

inline void print_reptest_results(
    repetition_test_results_t const &results,
    uint64_t target_processed_bytes,
    uint64_t cpu_timer_freq, char const *name,
    bool print_bytes_per_tick)
{
    long double const min_sec = (long double)results.min_ticks / cpu_timer_freq;
    long double const max_sec = (long double)results.max_ticks / cpu_timer_freq;

    uint64_t const avg_ticks = results.total_ticks / results.test_count;
    long double const avg_sec = ((long double)results.total_ticks / results.test_count) / cpu_timer_freq;

    RT_PRINTLN("");
    RT_PRINTLN("--- %s ---", name);

    RT_PRINT(
        "Min: %Lf (%llu), %Lfgb/s", min_sec, results.min_ticks,
        gb_per_measure(min_sec, target_processed_bytes));
    if (print_bytes_per_tick) {
        RT_PRINT(" (%.2Lfbytes/tick)",
            (long double)target_processed_bytes / results.min_ticks);
    }
    if (results.min_test_page_faults > 0) {
        RT_PRINT(
            " PF: %llu faults (%.3Lfkb/fault)", results.min_test_page_faults,
            c_kb_in_gb * gb_per_measure((long double)results.min_test_page_faults, target_processed_bytes));
    }
    RT_PRINTLN("");

    RT_PRINT(
        "Max: %Lf (%llu), %Lfgb/s", max_sec, results.max_ticks,
        gb_per_measure(max_sec, target_processed_bytes));
    if (print_bytes_per_tick) {
        RT_PRINT(" (%.2Lfbytes/tick)",
            (long double)target_processed_bytes / results.max_ticks);
    }
    if (results.max_test_page_faults > 0) {
        RT_PRINT(
            " PF: %llu faults (%.3Lfkb/fault)", results.max_test_page_faults,
            c_kb_in_gb * gb_per_measure((long double)results.max_test_page_faults, target_processed_bytes));
    }
    RT_PRINTLN("");

    RT_PRINT(
        "Avg: %Lf (%llu), %Lfgb/s", avg_sec, avg_ticks,
        gb_per_measure(avg_sec, target_processed_bytes));
    if (print_bytes_per_tick) {
        RT_PRINT(" (%.2Lfbytes/tick)",
            (long double)target_processed_bytes / avg_ticks);
    }
    if (results.total_page_faults > 0) {
        uint64_t const avg_page_faults = results.total_page_faults / results.test_count;
        RT_PRINT(
            " PF: %llu faults (%.3Lfkb/fault)", avg_page_faults,
            c_kb_in_gb * gb_per_measure((long double)avg_page_faults, target_processed_bytes));
    }
    RT_PRINTLN("");

    RT_PRINTLN("");
}

inline void print_best_bandwidth_csv(
    repetition_test_results_t const &results,
    uint64_t target_processed_bytes, uint64_t cpu_timer_freq,
    char const *label)
{
    long double const min_sec = (long double)results.min_ticks / cpu_timer_freq;
    RT_PRINTLN("%s,%llu,%Lf,%Lf",
        label, target_processed_bytes, min_sec,
        gb_per_measure(min_sec, target_processed_bytes));
}

inline long double best_gbps(
    repetition_test_results_t const &results,
    uint64_t target_processed_bytes, uint64_t cpu_timer_freq)
{
    long double const min_sec = (long double)results.min_ticks / cpu_timer_freq;
    return gb_per_measure(min_sec, target_processed_bytes);
}

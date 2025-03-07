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

    using stats_printer_t = void (*)(
        uint64_t /*min_ticks*/, uint64_t /*max_ticks*/,
        uint64_t /*total_ticks*/, uint64_t /*test_count*/,
        uint64_t /*processed_bytes*/, uint64_t /*cpu_timer_freq*/,
        uint64_t /*min_test_page_faults*/, uint64_t /*max_test_page_faults*/,
        uint64_t /*total_page_faults*/, bool /*print_bytes_per_tick*/,
        const char * /*name*/);

    uint64_t m_target_processed_bytes;
    uint64_t m_cpu_timer_freq;
    uint64_t m_try_renew_min_for_ticks;
    uint64_t m_test_start_ticks;

    uint64_t m_ticks;
    uint64_t m_page_faults;
    uint64_t m_bytes;

    uint32_t m_open_blocks;
    uint32_t m_closed_blocks;

    uint64_t m_test_count;
    uint64_t m_total_ticks;
    uint64_t m_total_bytes;
    uint64_t m_min_ticks;
    uint64_t m_max_ticks;
    uint64_t m_total_page_faults;
    uint64_t m_min_test_page_faults;
    uint64_t m_max_test_page_faults;

    uint32_t m_last_chars_printed_for_min;
    bool m_print_new_minimums;

    bool m_print_bytes_per_tick;

    const char *m_name;

    stats_printer_t m_stats_printer;

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
#define RT_PRINT(text_, ...) printf(text_, ##__VA_ARGS__)
#define RT_PRINTLN(text_, ...) printf(text_ "\n", ##__VA_ARGS__)
#define RT_CLEAR(count_)                         \
    do {                                         \
        for (size_t i_ = 0; i_ < (count_); ++i_) \
            putchar('\b');                       \
    } while (0)

  public:
    RepetitionTester(const char *name,
                     uint64_t target_bytes,
                     uint64_t cpu_timer_freq,
                     float seconds_for_min_renewal,
                     bool print_minimums = false,
                     bool print_bytes_per_tick = false,
                     stats_printer_t stats_printer = DefaultPrintStats)
        : m_state{e_st_pending}
        , m_target_processed_bytes{target_bytes}
        , m_cpu_timer_freq{cpu_timer_freq}
        , m_try_renew_min_for_ticks{uint64_t(seconds_for_min_renewal * cpu_timer_freq)}
        , m_print_new_minimums{print_minimums}
        , m_print_bytes_per_tick{print_bytes_per_tick}
        , m_name{name}
        , m_stats_printer{stats_printer} {}

    void ReStart() {
        if (m_state != e_st_pending)
            RT_ERR("Start called on non-pending rep tester");
        m_test_count = 0;
        m_total_ticks = 0;
        m_total_bytes = 0;
        m_total_page_faults = 0;
        m_min_ticks = uint64_t(-1);
        m_max_ticks = 0;
        m_min_test_page_faults = 0;
        m_max_test_page_faults = 0;
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
            if (m_bytes != m_target_processed_bytes)
                RT_ERRET(false, "Bytes processed in test not equal to target bytes");

            ++m_test_count;
            m_total_ticks += m_ticks;
            m_total_bytes += m_bytes;
            m_total_page_faults += m_page_faults;
            if (m_max_ticks < m_ticks) {
                m_max_ticks = m_ticks;
                m_max_test_page_faults = m_page_faults;
            }
            if (m_min_ticks > m_ticks) {
                m_min_ticks = m_ticks;
                m_min_test_page_faults = m_page_faults;
                m_test_start_ticks = cur_ticks;
                if (m_print_new_minimums) {
                    RT_CLEAR(m_last_chars_printed_for_min);
                    m_last_chars_printed_for_min = RT_PRINT(
                        "Found new min time: %Lfs",
                        (long double)m_min_ticks / m_cpu_timer_freq);
                }
            }
        }

        m_ticks = 0;
        m_bytes = 0;
        m_page_faults = 0;
        m_open_blocks = 0;
        m_closed_blocks = 0;

        if (cur_ticks - m_test_start_ticks > m_try_renew_min_for_ticks) {
            FinishAndPrintStats();
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

    // @HACK
    void OverrideTargetBytes(uint64_t bytes) { m_target_processed_bytes = bytes; }

    // @TODO format?
    void ReportError(const char *text) { RT_ERR("%s", text); }

    void SetName(const char *name) { m_name = name; }

private:
    void FinishAndPrintStats() {
        m_state = e_st_pending;
        m_stats_printer(m_min_ticks, m_max_ticks, m_total_ticks, m_test_count,
                        m_target_processed_bytes, m_cpu_timer_freq,
                        m_min_test_page_faults, m_max_test_page_faults,
                        m_total_page_faults, m_print_bytes_per_tick, m_name);
    }

    static void DefaultPrintStats(uint64_t min_ticks, uint64_t max_ticks,
                                  uint64_t total_ticks, uint64_t test_count,
                                  uint64_t processed_bytes, uint64_t cpu_timer_freq,
                                  uint64_t min_test_page_faults, uint64_t max_test_page_faults,
                                  uint64_t total_page_faults, bool print_bytes_per_tick,
                                  const char *name) {

        constexpr long double c_bytes_in_gb = (long double)(1u << 30);
        constexpr long double c_kb_in_gb = (long double)(1u << 20);

        // @TODO: pull out these computations
        auto gb_per_measure = [](long double measure, uint64_t bytes) {
            const long double gb = (long double)bytes / c_bytes_in_gb; 
            return gb / measure;
        };
        const long double min_sec = (long double)min_ticks / cpu_timer_freq;
        const long double max_sec = (long double)max_ticks / cpu_timer_freq;

        const uint64_t avg_ticks = total_ticks / test_count;
        const long double avg_sec = ((long double)total_ticks / test_count) / cpu_timer_freq;

        RT_PRINTLN("");
        RT_PRINTLN("--- %s ---", name);

        RT_PRINT("Min: %Lf (%llu), %Lfgb/s", min_sec, min_ticks,
                   gb_per_measure(min_sec, processed_bytes));
        if (print_bytes_per_tick)
            RT_PRINT(" (%.2Lfbytes/tick)", (long double)processed_bytes / min_ticks);
        if (min_test_page_faults > 0) {
            RT_PRINT(" PF: %llu faults (%.3Lfkb/fault)", min_test_page_faults,
                       c_kb_in_gb * gb_per_measure((long double)min_test_page_faults, processed_bytes));
        }
        RT_PRINTLN("");

        RT_PRINT("Max: %Lf (%llu), %Lfgb/s", max_sec, max_ticks,
                   gb_per_measure(max_sec, processed_bytes));
        if (print_bytes_per_tick)
            RT_PRINT(" (%.2Lfbytes/tick)", (long double)processed_bytes / max_ticks);
        if (max_test_page_faults > 0) {
            RT_PRINT(" PF: %llu faults (%.3Lfkb/fault)", max_test_page_faults,
                       c_kb_in_gb * gb_per_measure((long double)max_test_page_faults, processed_bytes));
        }
        RT_PRINTLN("");

        RT_PRINT("Avg: %Lf (%llu), %Lfgb/s", avg_sec, avg_ticks,
                   gb_per_measure(avg_sec, processed_bytes));
        if (print_bytes_per_tick)
            RT_PRINT(" (%.2Lfbytes/tick)", (long double)processed_bytes / avg_ticks);
        if (total_page_faults > 0) {
            const uint64_t avg_page_faults = total_page_faults / test_count;
            RT_PRINTLN(" PF: %llu faults (%.3Lfkb/fault)", avg_page_faults,
                       c_kb_in_gb * gb_per_measure((long double)avg_page_faults, processed_bytes));
        }
        RT_PRINTLN("");

        RT_PRINTLN("");
    }

#undef RT_ERR
#undef RT_ERRET
#undef RT_PRINTLN
#undef RT_PRINT
#undef RT_CLEAR
};


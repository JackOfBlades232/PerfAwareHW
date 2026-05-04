#pragma once

#include "defs.hpp"
#include "profiling.hpp"
#include "buffer.hpp"

#if !defined(RT_PRINT) || !defined(RT_PRINTLN)
#define RT_PRINT(fmt_, ...) fprintf(stderr, fmt_, ##__VA_ARGS__)
#define RT_PRINTLN(fmt_, ...) fprintf(stderr, fmt_ "\n", ##__VA_ARGS__)
#endif

#if !defined(RT_CLEAR) || !defined(RT_CLEARLN)
#define RT_CLEAR(count_)                         \
    do {                                         \
        for (usize i_ = 0; i_ < (count_); ++i_) \
            fprintf(stderr, "\b");               \
    } while (0)
#define RT_CLEARLN(count_)                       \
    do {                                         \
        for (usize i_ = 0; i_ < (count_); ++i_) \
            fprintf(stderr, "\b");               \
        for (usize i_ = 0; i_ < (count_); ++i_) \
            fprintf(stderr, " ");                \
    } while (0)
#endif

#define RT_PRESERVE_UNITS_TARGET u64(0)

enum repetition_test_units_t {
    e_rtu_bytes,
    e_rtu_ops,

    e_rtu_count
};

inline constexpr f64 (*c_rtu_giga_per_measure_funcs[e_rtu_count])(f64, u64) =
{
    &gb_per_measure,
    &gops_per_measure,
};

inline constexpr char const *c_rtu_shorthand_names_plural[e_rtu_count] =
{
    "bytes",
    "ops",
};

inline constexpr char const *c_rtu_kilo_shorthand_names_plural[e_rtu_count] =
{
    "kb",
    "kops",
};

inline constexpr char const *c_rtu_giga_shorthand_names_plural[e_rtu_count] =
{
    "gb",
    "gops",
};

inline constexpr u64 c_rtu_kilo_in_giga[e_rtu_count] =
{
    c_kb_in_gb,
    c_kops_in_gop,
};

struct repetition_test_results_t {
    u64 test_count = 0;
    u64 total_units = 0;
    u64 target_units = 0;
    u64 min_ticks = u64(-1);
    u64 max_ticks = 0;
    u64 total_ticks = 0;
    u64 min_test_page_faults = 0;
    u64 max_test_page_faults = 0;
    u64 total_page_faults = 0;
    repetition_test_units_t unit_type = e_rtu_bytes;
};

class RepetitionTester {
    enum state_t {
        e_st_pending,
        e_st_active,
        e_st_error
    } m_state;

    u64 m_target_units = 0;
    u64 m_try_renew_min_for_ticks = 0;
    u64 m_test_start_ticks = 0;
    u64 m_cpu_timer_freq = 0;

    u64 m_ticks = 0;
    u64 m_page_faults = 0;
    u64 m_units = 0;

    repetition_test_units_t m_unit_type = e_rtu_bytes;

    u32 m_open_blocks = 0;
    u32 m_closed_blocks = 0;

    u32 m_last_chars_printed_for_min = 0;
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
            u64 target_bytes, u64 cpu_timer_freq,
            f32 seconds_for_min_renewal,
            bool print_minimums = false,
            repetition_test_units_t unit_type = e_rtu_bytes)
            // @NOTE: I dislike this arg order, but I don't wanna correct code
        : m_state{e_st_pending}
        , m_target_units{target_bytes}
        , m_try_renew_min_for_ticks{u64(seconds_for_min_renewal * cpu_timer_freq)}
        , m_cpu_timer_freq{cpu_timer_freq}
        , m_unit_type{unit_type}
        , m_print_new_minimums{print_minimums}
        , m_results{nullptr} {}

    void ReStart(
        repetition_test_results_t &out_results,
        u64 new_target_units = RT_PRESERVE_UNITS_TARGET)
    {
        if (m_state != e_st_pending)
            RT_ERR("Start called on non-pending rep tester");
        if (new_target_units != RT_PRESERVE_UNITS_TARGET)
            m_target_units = new_target_units;
        m_results = &out_results;
        *m_results = repetition_test_results_t{};
        m_results->unit_type = m_unit_type;
        m_results->target_units = m_target_units;
        m_ticks = 0;
        m_page_faults = 0;
        m_units = 0;
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

        u64 cur_ticks = READ_TIMER();
        if (m_open_blocks) {
            if (m_open_blocks != m_closed_blocks)
                RT_ERRET(false, "Not all timed blocks closed in rep tester");
            if (m_units != m_target_units)
                RT_ERRET(false, "Bytes processed in test not equal to target bytes");

            ++m_results->test_count;
            m_results->total_ticks += m_ticks;
            m_results->total_units += m_units;
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
                        "Found new min time: %lfs",
                        large_divide(m_results->min_ticks, m_cpu_timer_freq));
                }
            }
        }

        m_ticks = 0;
        m_units = 0;
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

    void ReportProcessedUnits(u64 units) { m_units += units; }
    void ReportError(char const *text) { RT_ERR("%s", text); }

    u64 GetTargetUnits() const { return m_target_units; }

    // @NOTE: for compat with older code
    void ReportProcessedBytes(u64 bytes) {
        assert(m_unit_type == e_rtu_bytes);
        ReportProcessedUnits(bytes);
    }
    u64 GetTargetBytes() const {
        assert(m_unit_type == e_rtu_bytes);
        return GetTargetUnits();
    }

#undef RT_ERR
#undef RT_ERRET
#undef RT_CLEAR
};

inline void print_reptest_results(
    repetition_test_results_t const &results,
    u64 target_processed_units,
    u64 cpu_timer_freq, char const *name,
    bool print_units_per_tick)
{
    f64 const min_sec = large_divide(results.min_ticks, cpu_timer_freq);
    f64 const max_sec = large_divide(results.max_ticks, cpu_timer_freq);

    u64 const avg_ticks = results.total_ticks / results.test_count;
    f64 const avg_sec = large_divide(results.total_ticks, results.test_count) / cpu_timer_freq;

    auto gpm = c_rtu_giga_per_measure_funcs[results.unit_type];
    char const *mn = c_rtu_shorthand_names_plural[results.unit_type];
    char const *kmn = c_rtu_kilo_shorthand_names_plural[results.unit_type];
    char const *gmn = c_rtu_giga_shorthand_names_plural[results.unit_type];
    u64 kilo_in_giga = c_rtu_kilo_in_giga[results.unit_type];

    RT_PRINTLN("");
    RT_PRINTLN("--- %s ---", name);

    RT_PRINT(
        "Min: %lf (%llu), %lf%s/s", min_sec, results.min_ticks,
        gpm(min_sec, target_processed_units), gmn);
    if (print_units_per_tick) {
        RT_PRINT(" (%.2lf%s/tick)",
            large_divide(target_processed_units, results.min_ticks), mn);
    }
    if (results.min_test_page_faults > 0) {
        RT_PRINT(
            " PF: %llu faults (%.3lf%s/fault)", results.min_test_page_faults,
            kilo_in_giga * gpm(f64(results.min_test_page_faults), target_processed_units), kmn);
    }
    RT_PRINTLN("");

    RT_PRINT(
        "Max: %lf (%llu), %lf%s/s", max_sec, results.max_ticks,
        gpm(max_sec, target_processed_units), gmn);
    if (print_units_per_tick) {
        RT_PRINT(" (%.2lf%s/tick)",
            large_divide(target_processed_units, results.max_ticks), mn);
    }
    if (results.max_test_page_faults > 0) {
        RT_PRINT(
            " PF: %llu faults (%.3lf%s/fault)", results.max_test_page_faults,
            kilo_in_giga * gpm(f64(results.max_test_page_faults), target_processed_units), kmn);
    }
    RT_PRINTLN("");

    RT_PRINT(
        "Avg: %lf (%llu), %lf%s/s", avg_sec, avg_ticks,
        gb_per_measure(avg_sec, target_processed_units), gmn);
    if (print_units_per_tick) {
        RT_PRINT(" (%.2lf%s/tick)", large_divide(target_processed_units, avg_ticks), mn);
    }
    if (results.total_page_faults > 0) {
        u64 const avg_page_faults = results.total_page_faults / results.test_count;
        RT_PRINT(
            " PF: %llu faults (%.3lf%s/fault)", avg_page_faults,
            kilo_in_giga * gpm(f64(avg_page_faults), target_processed_units), kmn);
    }
    RT_PRINTLN("");

    RT_PRINTLN("");
}

// @NOTE: leaving for old code, legacy
inline void print_best_bandwidth_csv(
    repetition_test_results_t const &results,
    u64 target_processed_units, u64 cpu_timer_freq,
    char const *label)
{
    f64 const min_sec = large_divide(results.min_ticks, cpu_timer_freq);
    RT_PRINTLN("%s,%llu,%lf,%lf",
        label, target_processed_units, min_sec,
        c_rtu_giga_per_measure_funcs[results.unit_type](min_sec, target_processed_units));
}

inline f64 best_gps(
    repetition_test_results_t const &results,
    u64 target_processed_units, u64 cpu_timer_freq)
{
    f64 const min_sec = large_divide(results.min_ticks, cpu_timer_freq);
    return c_rtu_giga_per_measure_funcs[results.unit_type](min_sec, target_processed_units);
}

inline f64 best_ptick(
    repetition_test_results_t const &results, u64 target_processed_units)
{
    return large_divide(target_processed_units, results.min_ticks);
}

inline f64 best_pfpk(
    repetition_test_results_t const &results, u64 target_processed_units)
{
    return c_rtu_kilo_in_giga[results.unit_type] *
        c_rtu_giga_per_measure_funcs[results.unit_type](f64(results.min_test_page_faults), target_processed_units);
}

struct repetition_test_series_label_t {
    char namebuf[64];
};

struct repetition_test_series_t {
    repetition_test_series_label_t *rows_master_label;
    repetition_test_series_label_t *row_labels;
    repetition_test_series_label_t *col_labels;
    repetition_test_results_t *results;
    u32 col_count;
    u32 max_row_count;
    u32 current_row;
    u32 current_col;
    buffer_t membuf;
};

inline bool is_valid(repetition_test_series_t const &series)
{
    return is_valid(series.membuf);
}

inline repetition_test_series_t allocate_reptest_series(
    u32 col_count, u32 max_row_count)
{
    repetition_test_series_t series = {};

    u32 total_mem =
        sizeof(*series.rows_master_label) +
        sizeof(*series.row_labels) * max_row_count +
        sizeof(*series.col_labels) * col_count +
        sizeof(*series.results) * max_row_count * col_count;
    series.membuf = allocate_best(total_mem);
    if (!is_valid(series.membuf))
        return {};

    u8 *p = series.membuf.data;
    series.rows_master_label = (repetition_test_series_label_t *)p;
    p += sizeof(*series.rows_master_label);
    series.row_labels = (repetition_test_series_label_t *)p;
    p += sizeof(*series.row_labels) * max_row_count;
    series.col_labels = (repetition_test_series_label_t *)p;
    p += sizeof(*series.col_labels) * col_count;
    series.results = (repetition_test_results_t *)p;

    series.col_count = col_count;
    series.max_row_count = max_row_count;

    series.current_row = 0;
    series.current_col = 0;

    return series;
}

inline void free_reptest_series(repetition_test_series_t &series)
{
    if (!is_valid(series))
        return;
    deallocate(series.membuf);
    series = {};
}

inline void set_reptest_series_rows_master_label(
    repetition_test_series_t &series, char const *fmt, ...)
{
    assert(is_valid(series));
    series.current_col = series.col_count;
    va_list args;
    va_start(args, fmt);
    vsnprintf(
        series.rows_master_label->namebuf,
        sizeof(series.rows_master_label->namebuf),
        fmt, args);
    va_end(args);
}

inline void set_reptest_series_row_label(
    repetition_test_series_t &series, char const *fmt, ...)
{
    assert(is_valid(series));
    assert(series.current_row < series.max_row_count);
    assert(series.current_col == series.col_count);
    repetition_test_series_label_t
        *next_label = &series.row_labels[series.current_row++];
    series.current_col = 0;
    va_list args;
    va_start(args, fmt);
    vsnprintf(next_label->namebuf, sizeof(next_label->namebuf), fmt, args);
    va_end(args);
}

inline void set_reptest_series_col_label(
    repetition_test_series_t &series, char const *fmt, ...)
{
    assert(is_valid(series));
    assert(series.current_col < series.col_count);
    repetition_test_series_label_t
        *next_label = &series.col_labels[series.current_col++];
    va_list args;
    va_start(args, fmt);
    vsnprintf(next_label->namebuf, sizeof(next_label->namebuf), fmt, args);
    va_end(args);
}

inline void add_reptest_result_to_series(
    repetition_test_series_t &series, repetition_test_results_t const &result)
{
    assert(is_valid(series));
    assert(series.current_col > 0);
    assert(series.current_row > 0);
    assert(series.current_col <= series.col_count);
    assert(series.current_row <= series.max_row_count);

    u32 rowid = series.current_row - 1;
    u32 colid = series.current_col - 1;
    repetition_test_results_t
        *slot = &series.results[rowid * series.col_count + colid];

    *slot = result;
}

enum repetition_test_quantity_t {
    e_rtq_best_units_per_tick,
    e_rtq_best_gunits_per_sec,
    e_rtq_best_pfaults_per_kb
};

inline void dump_reptest_series_as_csv(
    repetition_test_series_t const &series,
    repetition_test_quantity_t quantity,
    u64 cpu_timer_freq,
    FILE *outfile = stdout)
{
    assert(is_valid(series));
    assert(series.current_col == series.col_count);
    fprintf(outfile, "%s", series.rows_master_label->namebuf);
    for (u32 i = 0; i < series.col_count; ++i)
        fprintf(outfile, ",%s", series.col_labels[i].namebuf);
    fprintf(outfile, "\n");
    for (u32 i = 0; i < series.current_row; ++i) {
        fprintf(outfile, "%s", series.row_labels[i].namebuf);
        for (u32 j = 0; j < series.col_count; ++j) {
            repetition_test_results_t const &res =
                series.results[i * series.col_count + j];
            f64 q = 0.0;
            switch (quantity) {
            case e_rtq_best_units_per_tick:
                q = best_ptick(res, res.target_units);
                break;
            case e_rtq_best_gunits_per_sec:
                q = best_gps(res, res.target_units, cpu_timer_freq);
                break;
            case e_rtq_best_pfaults_per_kb:
                q = best_pfpk(res, res.target_units);
                break;
            }
            fprintf(outfile, ",%lf", q);
        }
        fprintf(outfile, "\n");
    }
}

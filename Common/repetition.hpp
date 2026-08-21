#pragma once

#include "defs.hpp"
#include "profiling.hpp"
#include "buffer.hpp"

inline void rt_clear(FILE *f, usize len)
{
    for (usize i = 0; i < len; ++i)
        fprintf(f, "\b");
}

inline void rt_clearln(FILE *f, usize len)
{
    rt_clear(f, len);
    for (usize i = 0; i < len; ++i)
        fprintf(f, " ");
}

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

struct repetition_test_processed_units_record_t {
    u64 total_units = 0;
    u64 target_units = 0;
};

struct repetition_test_results_t {
    u64 test_count = 0;
    u64 min_ticks = u64(-1);
    u64 max_ticks = 0;
    u64 total_ticks = 0;
    u64 min_test_page_faults = 0;
    u64 max_test_page_faults = 0;
    u64 total_page_faults = 0;
    repetition_test_processed_units_record_t unit_results[e_rtu_count] = {};
};

inline void set_rtr_target_units(
    repetition_test_results_t &results, repetition_test_units_t type, u64 value)
{
    results.unit_results[type].target_units = value;
}

inline void set_rtr_target_bytes(repetition_test_results_t &results, u64 bytes)
{
    set_rtr_target_units(results, e_rtu_bytes, bytes);
}

inline void set_rtr_target_ops(repetition_test_results_t &results, u64 ops)
{
    set_rtr_target_units(results, e_rtu_ops, ops);
}

class RepetitionTester {
    enum state_t {
        e_st_pending,
        e_st_active,
        e_st_error
    } m_state;

    u64 m_try_renew_min_for_ticks = 0;
    u64 m_test_start_ticks = 0;
    u64 m_cpu_timer_freq = 0;

    u64 m_ticks = 0;
    u64 m_page_faults = 0;

    u64 m_units[e_rtu_count] = {};

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
            u64 cpu_timer_freq,
            f32 seconds_for_min_renewal,
            bool print_minimums = false)
            // @NOTE: I dislike this arg order, but I don't wanna correct code
        : m_state{e_st_pending}
        , m_try_renew_min_for_ticks{u64(seconds_for_min_renewal * cpu_timer_freq)}
        , m_cpu_timer_freq{cpu_timer_freq}
        , m_print_new_minimums{print_minimums}
        , m_results{nullptr} {}

    void ReStart(repetition_test_results_t &out_results)
    {
        if (m_state != e_st_pending)
            RT_ERR("Start called on non-pending rep tester");
        m_results = &out_results;
        u64 saved_targets[e_rtu_count] = {};
        for (u32 i = 0; i < e_rtu_count; ++i)
            saved_targets[i] = m_results->unit_results[i].target_units;
        *m_results = repetition_test_results_t{};
        for (u32 i = 0; i < e_rtu_count; ++i)
            m_results->unit_results[i].target_units = saved_targets[i];
        m_ticks = 0;
        m_page_faults = 0;
        m_open_blocks = 0;
        m_closed_blocks = 0;
        m_last_chars_printed_for_min = 0;
        m_state = e_st_active;
        memset(m_units, 0, sizeof(m_units));
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
            for (u32 i = 0; i < e_rtu_count; ++i) {
                repetition_test_processed_units_record_t *slot =
                    &m_results->unit_results[i];
                if (m_units[i] != slot->target_units) {
                    RT_ERRET(false,
                        "Bytes processed in test not equal to target bytes");
                }
                slot->total_units += m_units[i];
            }

            ++m_results->test_count;
            m_results->total_ticks += m_ticks;
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
                    rt_clear(stderr, m_last_chars_printed_for_min);
                    m_last_chars_printed_for_min = fprintf(stderr,
                        "Found new min time: %lfs",
                        large_divide(m_results->min_ticks, m_cpu_timer_freq));
                }
            }
        }

        m_ticks = 0;
        m_page_faults = 0;
        m_open_blocks = 0;
        m_closed_blocks = 0;
        memset(m_units, 0, sizeof(m_units));

        if (cur_ticks - m_test_start_ticks > m_try_renew_min_for_ticks) {
            if (m_print_new_minimums)
                rt_clearln(stderr, m_last_chars_printed_for_min);
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

    void ReportProcessedUnits(repetition_test_units_t type, u64 value)
        { m_units[type] += value; }
    void ReportProcessedBytes(u64 bytes)
        { ReportProcessedUnits(e_rtu_bytes, bytes); }
    void ReportProcessedOps(u64 ops)
        { ReportProcessedUnits(e_rtu_ops, ops); }

    void ReportError(char const *text) { RT_ERR("%s", text); }

#undef RT_ERR
#undef RT_ERRET
};

inline void print_reptest_results(
    repetition_test_results_t const &results,
    u64 cpu_timer_freq, char const *name,
    bool print_units_per_tick)
{
    f64 const min_sec = large_divide(results.min_ticks, cpu_timer_freq);
    f64 const max_sec = large_divide(results.max_ticks, cpu_timer_freq);

    u64 const avg_ticks = results.total_ticks / results.test_count;
    f64 const avg_sec = large_divide(results.total_ticks, results.test_count) / cpu_timer_freq;

    fprintf(stderr, "\n");
    fprintf(stderr, "--- %s ---\n", name);

    fprintf(stderr, "Min: %lf (%llu)", min_sec, results.min_ticks);
    for (u32 i = 0; i < e_rtu_count; ++i) {
        u64 const  target = results.unit_results[i].target_units;
        if (!target)
            continue;
        fprintf(stderr, " %lf%s/s",
            c_rtu_giga_per_measure_funcs[i](min_sec, target),
            c_rtu_giga_shorthand_names_plural[i]);
        if (print_units_per_tick) {
            fprintf(stderr, " (%.2lf%s/tick)",
                large_divide(target, results.min_ticks),
                c_rtu_shorthand_names_plural[i]);
        }
    }
    if (results.min_test_page_faults > 0) {
        fprintf(stderr, " PF: %llu faults", results.min_test_page_faults);
        for (u32 i = 0; i < e_rtu_count; ++i) {
            u64 const  target = results.unit_results[i].target_units;
            if (!target)
                continue;
            fprintf(stderr, 
                " (%.3lf%s/fault)",
                c_rtu_kilo_in_giga[i] * c_rtu_giga_per_measure_funcs[i](
                    f64(results.min_test_page_faults), target),
                c_rtu_kilo_shorthand_names_plural[i]);
        }
    }
    fprintf(stderr, "\n");

    fprintf(stderr, "Max: %lf (%llu)", max_sec, results.max_ticks);
    for (u32 i = 0; i < e_rtu_count; ++i) {
        u64 const  target = results.unit_results[i].target_units;
        if (!target)
            continue;
        fprintf(stderr, " %lf%s/s",
            c_rtu_giga_per_measure_funcs[i](max_sec, target),
            c_rtu_giga_shorthand_names_plural[i]);
        if (print_units_per_tick) {
            fprintf(stderr, " (%.2lf%s/tick)",
                large_divide(target, results.max_ticks),
                c_rtu_shorthand_names_plural[i]);
        }
    }
    if (results.max_test_page_faults > 0) {
        fprintf(stderr, " PF: %llu faults", results.max_test_page_faults);
        for (u32 i = 0; i < e_rtu_count; ++i) {
            u64 const  target = results.unit_results[i].target_units;
            if (!target)
                continue;
            fprintf(stderr, 
                " (%.3lf%s/fault)",
                c_rtu_kilo_in_giga[i] * c_rtu_giga_per_measure_funcs[i](
                    f64(results.max_test_page_faults), target),
                c_rtu_kilo_shorthand_names_plural[i]);
        }
    }
    fprintf(stderr, "\n");

    fprintf(stderr, "Avg: %lf (%llu)", avg_sec, avg_ticks);
    for (u32 i = 0; i < e_rtu_count; ++i) {
        u64 const  target = results.unit_results[i].target_units;
        if (!target)
            continue;
        fprintf(stderr, " %lf%s/s",
            c_rtu_giga_per_measure_funcs[i](avg_sec, target),
            c_rtu_giga_shorthand_names_plural[i]);
        if (print_units_per_tick) {
            fprintf(stderr, " (%.2lf%s/tick)",
                large_divide(target, avg_ticks),
                c_rtu_shorthand_names_plural[i]);
        }
    }
    fprintf(stderr, "\n");
}

inline f64 best_gps(
    repetition_test_results_t const &results,
    repetition_test_units_t units,
    u64 cpu_timer_freq)
{
    f64 const min_sec = large_divide(results.min_ticks, cpu_timer_freq);
    return c_rtu_giga_per_measure_funcs[units](
        min_sec, results.unit_results[units].target_units);
}

inline f64 best_ptick(
    repetition_test_results_t const &results,
    repetition_test_units_t units)
{
    return large_divide(
        results.unit_results[units].target_units, results.min_ticks);
}

inline f64 best_pfpk(
    repetition_test_results_t const &results,
    repetition_test_units_t units)
{
    return c_rtu_kilo_in_giga[units] * c_rtu_giga_per_measure_funcs[units](
        f64(results.min_test_page_faults),
        results.unit_results[units].target_units);
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
    repetition_test_units_t units,
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
                q = best_ptick(res, units);
                break;
            case e_rtq_best_gunits_per_sec:
                q = best_gps(res, units, cpu_timer_freq);
                break;
            case e_rtq_best_pfaults_per_kb:
                q = best_pfpk(res, units);
                break;
            }
            fprintf(outfile, ",%lf", q);
        }
        fprintf(outfile, "\n");
    }
}

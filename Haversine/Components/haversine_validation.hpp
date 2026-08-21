#pragma once

#include "haversine_state.hpp"
#include "haversine_common.hpp"

#include <logging.hpp>

struct haversine_validation_result_t {
    f64 sum_error = 0.0;
    f64 avg_error = 0.0;
    f64 max_error = 0.0;
    u64 answer_count_above_avg_error = 0;
    u64 answer_count_above_float_eps = 0;
};

inline haversine_validation_result_t
validate_haversine_distances(haversine_state_t const &s)
{
    assert(s.validation_answers);
    haversine_validation_result_t results = {};
    for (u32 i = 0; i < s.pair_cnt; ++i) {
        f64 const ans = s.answers[i];
        f64 const ref = s.validation_answers[i];
        f64 const error = abs(ans - ref);
        results.avg_error += error;
        results.max_error = max(results.max_error, error);
        if (error > FLT_EPSILON)
            ++results.answer_count_above_float_eps;
    }
    results.avg_error /= f64(s.pair_cnt);
    results.sum_error = abs(s.sum_answer - s.validation_sum);
    for (u32 i = 0; i < s.pair_cnt; ++i) {
        f64 const ans = s.answers[i];
        f64 const ref = s.validation_answers[i];
        f64 const error = abs(ans - ref);
        if (error > results.avg_error)
            ++results.answer_count_above_avg_error;
    }
    return results;
}

inline void print_haversine_validation_results(
    haversine_validation_result_t const &results)
{
    fprintf(stderr,
        "CsumError=%.16g AvgError=%.16g MaxError=%.16g "
        "ErrorsAboveAvg=%lu ErrorsAboveFltEps=%lu\n",
        results.sum_error, results.avg_error, results.max_error,
        results.answer_count_above_avg_error,
        results.answer_count_above_float_eps);
}

inline void merge_worst_haversine_validation_result(
    haversine_validation_result_t &accum,
    haversine_validation_result_t const &new_result)
{
    accum.sum_error = max(accum.sum_error, new_result.sum_error);
    accum.avg_error = max(accum.avg_error, new_result.avg_error);
    accum.max_error = max(accum.max_error, new_result.max_error);
    accum.answer_count_above_avg_error = max(
        accum.answer_count_above_avg_error,
        new_result.answer_count_above_avg_error);
    accum.answer_count_above_float_eps = max(
        accum.answer_count_above_float_eps,
        new_result.answer_count_above_float_eps);
}

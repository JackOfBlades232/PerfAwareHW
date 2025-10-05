#pragma once

#include "haversine_state.hpp"
#include "haversine_common.hpp"

#include <logging.hpp>

// @TODO: make as strong as should be
inline bool validate_value(f64 res, f64 ref)
{
    return abs(res - ref) <= 1e-5;
}

inline bool validate_haversine_distances(
    haversine_state_t &s, bool log_if_ok = false)
{
    if (log_if_ok) {
        LOGNORMAL("Point count: %lu", s.pair_cnt);
        LOGNORMAL("Avg dist: %.18lg\n", s.sum_answer / s.pair_cnt);
    }

    if (s.validation_answers) {
        u32 missed_answers = 0;
        for (u32 i = 0; i < s.pair_cnt; ++i) {
            if (!validate_value(s.answers[i], s.validation_answers[i]))
                ++missed_answers;
        }

        if (!validate_value(s.sum_answer, s.validation_sum)) {
            LOGERR(
                "Validation failed: "
                "claculated sum = %.18lg, required = %.18lg, "
                "missed %u/%lu answers, SumError=%.18lg",
                s.sum_answer, s.validation_sum, missed_answers,
                s.pair_cnt, abs(s.validation_sum - s.sum_answer));
            return false;
        } else if (log_if_ok) {
            LOGNORMAL(
                "Validation: Avg=%.18lg, "
                "%u/%lu answers inexact, SumError=%.18lg",
                s.validation_sum / s.pair_cnt, missed_answers,
                s.pair_cnt, abs(s.validation_sum - s.sum_answer));
        }
    } else if (log_if_ok) {
        LOGNORMAL("Validation file not found");
    }

    return true;
}

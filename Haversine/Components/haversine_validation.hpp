#pragma once

#include "haversine_state.hpp"
#include "haversine_common.hpp"

#include <logging.hpp>

inline bool validate_haversine_distances(
    haversine_state_t &s, bool log_if_ok = false)
{
    if (log_if_ok) {
        LOGNORMAL("Point count: %lu", s.pair_cnt);
        LOGNORMAL("Avg dist: %f\n", s.sum_answer / s.pair_cnt);
    }

    if (s.validation_answers) {
        u32 missed_answers = 0;
        for (u32 i = 0; i < s.pair_cnt; ++i) {
            if (s.answers[i] != s.validation_answers[i])
                ++missed_answers;
        }

        if (s.sum_answer != s.validation_sum || missed_answers) {
            LOGERR(
                "Validation failed: "
                "claculated sum = %f, required = %f, missed %u answers",
                s.sum_answer, s.validation_sum, missed_answers);
            return false;
        } else if (log_if_ok) {
            LOGNORMAL("Validation: %f", s.validation_sum / s.pair_cnt);
        }
    } else if (log_if_ok) {
        LOGNORMAL("Validation file not found");
    }

    return true;
}

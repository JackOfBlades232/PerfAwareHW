#include <haversine_state.hpp>
#include <haversine_calculation.hpp>
#include <haversine_validation.hpp>

#include <os.hpp>
#include <logging.hpp>

int main(int argc, char **argv)
{
    if (argc < 2) {
        LOGERR("Usage: <program> [fname]");
        return 1;
    }

    char const *json_fn = argv[1];
    init_os_process_state(g_os_proc_state);

    haversine_state_t state = {};
    if (!setup_haversine_state(state, json_fn))
        return 2;
    DEFER([&] { cleanup_haversine_state(state); });

    haversine_function_input_ranges_t ranges = {};

    calculate_haversine_distances(state,
        [&](point_pair_t pair) {
            return haversine_dist_range_check(pair, ranges);
        });

    if (!validate_haversine_distances(state))
        return 3;

    LOGNORMAL("Cos: [%f, %f]",
        ranges.cos_input_range.min, ranges.cos_input_range.max);
    LOGNORMAL("Asin: [%f, %f]",
        ranges.asin_input_range.min, ranges.asin_input_range.max);
    LOGNORMAL("Sqrt: [%f, %f]",
        ranges.sqrt_input_range.min, ranges.sqrt_input_range.max);
}

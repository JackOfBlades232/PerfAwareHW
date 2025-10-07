#include <haversine_state.hpp>
#include <haversine_calculation.hpp>
#include <haversine_json_parser.hpp>
#include <haversine_file_io.hpp>
#include <haversine_validation.hpp>

#include <string.hpp>
#include <defer.hpp>
#include <profiling.hpp>
#include <os.hpp>
#include <defs.hpp>
#include <memory.hpp>

int main(int argc, char **argv)
{
    init_os_process_state(g_os_proc_state);
    try_enable_large_pages(g_os_proc_state);

    init_profiler();
    DEFER([] { finish_profiling_and_dump_stats(printf); });

    PROFILED_BLOCK_PF("Program");

    bool only_tokenize = false;
    bool only_reprint_json = false;
    char const *json_fname = nullptr;

    {
        PROFILED_BLOCK_PF("Argument parsing");

        for (int i = 1; i < argc; ++i) {
            if (streq(argv[i], "-f")) {
                ++i;
                if (i >= argc) {
                    LOGERR("Invalid arg, specify file name after -f");
                    return 1;
                }

                json_fname = argv[i];
            } else if (streq(argv[i], "-tokenize")) {
                if (only_reprint_json) {
                    LOGERR(
                        "Invalid usage: "
                        "-tokenize and -reprint are incompatible");
                    return 1;
                }
                only_tokenize = true;
            } else if (streq(argv[i], "-reprint")) {
                if (only_tokenize) {
                    LOGERR(
                        "Invalid usage: "
                        "-tokenize and -reprint are incompatible");
                    return 1;
                }
                only_reprint_json = true;
            } else {
                LOGERR("Invalid arg: %s", argv[i]);
                return 1;
            }
        }
    }

    if (!json_fname) {
        LOGERR("Invalid usage: specify input file with -f <name>");
        return 1;
    }

    haversine_state_t state = {};

    if (!setup_haversine_state(state, json_fname, only_tokenize))
        return 2;

    DEFER([&] { cleanup_haversine_state(state); });

    if (only_tokenize)
        return tokenize_and_print(state.json_source_buffer);
    if (only_reprint_json)
        return reprint_json(state.parsed_json_root);

    calculate_haversine_distances_inline(state);

    if (!validate_haversine_distances(state, true))
        return 1;
}

static_assert(
    __COUNTER__ < c_profiler_slots_count,
    "Too many profiler slots declared");

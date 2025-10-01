#include <haversine_file_io.hpp>
#include <haversine_json_parser.hpp>

#include <string.hpp>
#include <defer.hpp>
#include <profiling.hpp>
#include <os.hpp>
#include <defs.hpp>
#include <memory.hpp>

struct point_pair_t {
    f32 x0, y0, x1, y1;
};

static f32 haversine_dist(point_pair_t pair)
{
    PROFILED_FUNCTION;

    constexpr f32 c_pi = 3.14159265359f;

    auto deg2rad = [](f32 deg) { return deg * c_pi / 180.f; };
    auto rad2deg = [](f32 rad) { return rad * 180.f / c_pi; };

    auto haversine = [](f32 rad) { return (1.f - cosf(rad)) * 0.5f; };

    f32 dx = deg2rad(fabsf(pair.x0 - pair.x1));
    f32 dy = deg2rad(fabsf(pair.y0 - pair.y1));
    f32 y0r = deg2rad(pair.y0);
    f32 y1r = deg2rad(pair.y1);

    f32 hav_of_diff =
        haversine(dy) + cosf(y0r)*cosf(y1r)*haversine(dx);

    f32 rad_of_diff = acosf(1.f - 2.f*hav_of_diff);
    return rad2deg(rad_of_diff);
}

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

    input_file_t inf = {};
    buffer_t checksum_data = {};

    DEFER([&] { deallocate(inf.source); });
    DEFER([&] { deallocate(checksum_data); });

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

                inf.source = load_entire_file(json_fname);
                if (!is_valid(inf.source)) {
                    LOGERR("Failed to load json file '%s'", json_fname);
                    return 1;
                }
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

    if (!is_valid(inf.source)) {
        LOGERR("Invalid usage: specify input file with -f <name>");
        return 1;
    }

    if (only_tokenize)
        return tokenize_and_print(inf);

    json_ent_t *root = parse_json_input(inf);
    if (!root)
        return 2;

    DEFER([&root] {
        PROFILED_BLOCK_PF("Delete json");
        deallocate(root);
    });

    if (only_reprint_json)
        return reprint_json(root);

    {
        PROFILED_BLOCK_PF("Misc preparation");

        if (root->obj.field_cnt != 1 ||
            !streq(root->obj.fields[0].name.s, "points") ||
            root->obj.fields[0].ent->type != e_jt_array)
        {
            LOGERR("Invalid format: correct is { \"points\": [ ... ] }");
            return 2;
        }

        if (json_fname) {
            char checksum_fname[256];
            snprintf(
                checksum_fname, sizeof(checksum_fname),
                "%s.check.bin", json_fname);
            checksum_data = load_entire_file(checksum_fname);
            if (!is_valid(checksum_data)) {
                LOGDBG(
                    "Failed to load checksum file from '%s', no validation",
                    checksum_fname);
            }
        }
    }

    json_array_t &points_arr = root->obj.fields[0].ent->arr;
    json_ent_t *const *points = points_arr.elements;
    u32 const point_cnt = points_arr.element_cnt;
    point_pair_t *pairs =
        (point_pair_t *)malloc(point_cnt * sizeof(point_pair_t));

    {
        PROFILED_BLOCK_PF("Haversine parsing");

        for (u32 i = 0; i < point_cnt; ++i) {
            json_ent_t const *elem = points[i];
            auto print_point_format_error = [] {
                LOGERR(
                    "Invalid point format: correct is "
                    "{ \"x0\": .f, \"y0\": .f, \"x1\": .f, \"y1\": .f }");
            };
            if (elem->type != e_jt_object || elem->obj.field_cnt != 4) {
                print_point_format_error();
                return 2;
            }

            auto read_f32 = [elem](char const *name, f32 &out) {
                if (json_ent_t const *d = json_object_query(*elem, name)) {
                    if (d->type == e_jt_number) {
                        out = f32(d->num);
                        return true;
                    }
                }
                return false;
            };
            f32 x0, y0, x1, y1;
            if (!read_f32("x0", x0) ||
                !read_f32("y0", y0) ||
                !read_f32("x1", x1) ||
                !read_f32("y1", y1))
            {
                print_point_format_error();
                return 2;
            }

            pairs[i] = {x0, y0, x1, y1};
        }
    }

    f32 sum;
    f32 avg;

    {
        PROFILED_BANDWIDTH_BLOCK_PF(
            "Haversine claculation", point_cnt * sizeof(point_pair_t));

        sum = 0.f;

        for (u32 i = 0; i < point_cnt; ++i) {
            f32 const dist = haversine_dist(pairs[i]);

            if (is_valid(checksum_data)) {
                f32 valid = ((f32 *)checksum_data.data)[i];
                if (dist != valid) {
                    LOGERR(
                        "Validation failed: "
                        "dist #%u mismatch, claculated %f, required %f",
                        i, dist, valid);
                    return 2;
                }
            }

            sum += dist;
        }

        avg = sum / point_cnt;
    }

    {
        PROFILED_BLOCK_PF("Output, validation and shutdown");

        OUTPUT("Point count: %u\n", point_cnt);
        OUTPUT("Avg dist: %f\n\n", avg);

        if (is_valid(checksum_data)) {
            f32 valid = ((f32 *)checksum_data.data)[point_cnt];
            if (avg != valid) {
                LOGERR(
                    "Validation failed: "
                    "avg dist mismatch, claculated %f, required %f",
                    avg, valid);
                return 2;
            } else {
                OUTPUT("Validation: %f\n", valid);
            }
        } else {
            OUTPUT("Validation file not found\n");
        }
    }

    return 0;
}

static_assert(
    __COUNTER__ < c_profiler_slots_count,
    "Too many profiler slots declared");

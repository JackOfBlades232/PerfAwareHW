#pragma once

#include "haversine_common.hpp"

#include "haversine_file_io.hpp"
#include "haversine_json_parser.hpp"

#include <buffer.hpp>
#include <profiling.hpp>

struct point_pair_t {
    f32 x0, y0, x1, y1;
};

struct haversine_state_t {
    buffer_t json_source_buffer;
    buffer_t checksum_buffer;
    buffer_t parsed_pairs_buffer;
    buffer_t answers_buffer;

    u64 parsed_byte_count;

    json_ent_t *parsed_json_root;

    point_pair_t *pairs;
    f32 *answers;
    u64 pair_cnt;

    f32 sum_answer;

    f32 *validation_answers;
    f32 validation_sum;

    bool valid;
};

void cleanup_haversine_state(haversine_state_t &s)
{
    PROFILED_FUNCTION;

    deallocate(s.json_source_buffer);
    deallocate(s.checksum_buffer);
    deallocate(s.parsed_pairs_buffer);
    deallocate(s.answers_buffer);
    deallocate(s.parsed_json_root); 
    s = {};
}

bool setup_haversine_state(
    haversine_state_t &s, char const *json_fn, bool only_load_json = false)
{
    assert(json_fn);

    PROFILED_FUNCTION_PF;

    cleanup_haversine_state(s);

    s.json_source_buffer = load_entire_file(json_fn);
    if (!is_valid(s.json_source_buffer)) {
        LOGERR("Failed to load json file '%s'", json_fn);
        return false;
    }

    if (only_load_json)
        return true;

    if (!(s.parsed_json_root = parse_json_input(s.json_source_buffer))) {
        cleanup_haversine_state(s);
        return false;
    }
    
    {
        PROFILED_BLOCK_PF("Misc preparation");

        if (s.parsed_json_root->obj.field_cnt != 1 ||
            !streq(s.parsed_json_root->obj.fields[0].name.s, "points") ||
            s.parsed_json_root->obj.fields[0].ent->type != e_jt_array)
        {
            LOGERR("Invalid format: correct is { \"points\": [ ... ] }");
            cleanup_haversine_state(s);
            return false;
        }

        char checksum_fn[256];
        snprintf(
            checksum_fn, sizeof(checksum_fn),
            "%s.check.bin", json_fn);
        s.checksum_buffer = load_entire_file(checksum_fn);
        if (!is_valid(s.checksum_buffer)) {
            LOGDBG(
                "Failed to load checksum file from '%s', no validation",
                checksum_fn);
        }
    }

    json_array_t &points_arr = s.parsed_json_root->obj.fields[0].ent->arr;
    json_ent_t *const *points = points_arr.elements;

    s.pair_cnt = points_arr.element_cnt;

    s.parsed_pairs_buffer = allocate_best(s.pair_cnt * sizeof(point_pair_t));
    s.pairs = (point_pair_t *)s.parsed_pairs_buffer.data;

    s.answers_buffer = allocate_best(s.pair_cnt * sizeof(f32));
    s.answers = (f32 *)s.answers_buffer.data;

    if (is_valid(s.checksum_buffer)) {
        if (s.checksum_buffer.len != (s.pair_cnt + 1) * sizeof(f32)) {
            LOGERR("Invalid checksum file '%s.check.bin'", json_fn);
            cleanup_haversine_state(s);
            return false;
        }
        
        s.validation_answers = (f32 *)s.checksum_buffer.data;
        s.validation_sum = s.validation_answers[s.pair_cnt];
    }

    {
        PROFILED_BLOCK_PF("Haversine parsing");

        for (u32 i = 0; i < s.pair_cnt; ++i) {
            json_ent_t const *elem = points[i];
            auto print_point_format_error = [] {
                LOGERR(
                    "Invalid point format: correct is "
                    "{ \"x0\": .f, \"y0\": .f, \"x1\": .f, \"y1\": .f }");
            };
            if (elem->type != e_jt_object || elem->obj.field_cnt != 4) {
                print_point_format_error();
                cleanup_haversine_state(s);
                return false;
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
                cleanup_haversine_state(s);
                return false;
            }

            s.pairs[i] = {x0, y0, x1, y1};
        }
    }

    return true;
}

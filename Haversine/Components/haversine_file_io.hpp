#pragma once

#include <buffer.hpp>
#include <defer.hpp>
#include <memory.hpp>
#include <files.hpp>
#include <logging.hpp>

// @TODO: implement chunked processing for windows, mmap for linux

inline usize get_file_len(char const *fn)
{
    os_file_t f = os_read_open_file(fn);
    if (!is_valid(f))
        return 0;
    DEFER([&f] { os_close_file(f); });

    return f.len;
}

inline buffer_t load_entire_file(char const *fn)
{
#if PROFILER
    usize const bytes = get_file_len(fn);
    PROFILED_BANDWIDTH_FUNCTION_PF(bytes);
#endif

    os_file_t of = os_read_open_file(fn);
    if (!is_valid(of))
        return {};
    DEFER([&of] { os_close_file(of); });

    buffer_t b = {};

    if (is_valid(b = allocate_lp(of.len))) {
        LOGDBG("Allocated '%s' with large pages", fn);
    } else if (is_valid(b = allocate(of.len))) {
        LOGDBG("Allocated '%s' with regular pages", fn);
    } else {
        return {};
    }

    if (os_file_read(of, b.data, b.len) != b.len) {
        deallocate(b);
        return {};
    }

    return b;
}

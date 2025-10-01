#pragma once

#include <defs.hpp>
#include <memory.hpp>
#include <profiling.hpp>

struct buffer_t {
    u8 *data;
    u64 len : 63;
    bool is_large_pages : 1;
};

inline bool is_valid(buffer_t const &b)
{
    return b.data;
}

inline buffer_t allocate(u64 bytes)
{
    PROFILED_BANDWIDTH_FUNCTION(bytes);
    void *data = allocate_os_pages_memory(bytes);
    return{(u8 *)data, bytes, false};
}

inline buffer_t allocate_lp(u64 bytes)
{
    PROFILED_BANDWIDTH_FUNCTION(bytes);
    void *data = allocate_os_large_pages_memory(bytes);
    return{(u8 *)data, bytes, true};
}

inline buffer_t allocate_best(u64 bytes)
{
    PROFILED_BANDWIDTH_FUNCTION(bytes);
    if (buffer_t b = allocate_lp(bytes); is_valid(b))
        return b;
    else
        return allocate(bytes);
}

inline void deallocate(buffer_t &buf)
{
    if (buf.data) {
        PROFILED_BANDWIDTH_FUNCTION(buf.len);
        (buf.is_large_pages ?
            &free_os_large_pages_memory :
            &free_os_pages_memory)(buf.data, buf.len);
        buf = {};
    }
}

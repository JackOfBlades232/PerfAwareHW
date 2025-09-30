#pragma once

#include <buffer.hpp>

struct string_t {
    char *s;
    u64 len;
};

inline string_t str_from_buf(buffer_t buf)
{
    return {(char *)buf.data, buf.len};
}

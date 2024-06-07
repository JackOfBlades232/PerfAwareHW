#pragma once
#include "defs.hpp"

struct memory_access_t {
    u8 *mem;
    u32 base;
    u32 size;
};

u32 load_file_to_memory(memory_access_t dest, const char *fn);

#pragma once
#include "defs.hpp"

struct memory_access_t {
    u8 *mem;
    u32 base;
    u32 size;
};

// @TODO: now I just load to the beginning of the memory, it would be better to change that
u32 load_file_to_memory(memory_access_t dest, const char *fn);

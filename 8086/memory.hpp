#pragma once
#include "defs.hpp"

struct memory_access_t {
    u8 *mem;
    u32 base;
    u32 size;
};

memory_access_t get_main_memory_access();
u8 get_byte_at(memory_access_t at, u32 offset);

u32 load_file_to_memory(memory_access_t dest, const char *fn);

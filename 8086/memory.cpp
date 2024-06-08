#include "memory.hpp"
#include "defs.hpp"
#include "util.hpp"
#include <cstdio>

static constexpr u32 c_mem_size = POT(20);
static constexpr u32 c_mem_mask = c_mem_size - 1;

static u8 g_memory[c_mem_size];

memory_access_t get_main_memory_access()
{
    return {g_memory, 0, c_mem_size};
}

u8 get_byte_at(memory_access_t at, u32 offset)
{
    // @TODO: disallow access beyond size
    u32 id = (at.base + offset) & c_mem_mask;
    return at.mem[id];
}

// @TODO: now I just load to the beginning of the memory, it would be better to change that
u32 load_file_to_memory(memory_access_t dest, const char *fn)
{
    FILE *f = fopen(fn, "rb");
    if (!f) {
        LOGERR("Failed to open file %s for load", fn);
        return 0;
    }

    // @NOTE: if the file is too large, it is just truncated
    u32 bytes_read = fread(dest.mem + dest.base, 1, dest.size, f);
    fclose(f);
    return bytes_read;
}

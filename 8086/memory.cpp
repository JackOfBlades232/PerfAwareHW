#include "memory.hpp"
#include "defs.hpp"
#include "util.hpp"
#include <cstdio>

static constexpr u32 c_mem_size = POT(20);
static constexpr u32 c_mem_mask = c_mem_size - 1;

static u8 g_memory[c_mem_size] = {};

memory_access_t get_main_memory_access()
{
    return {g_memory, 0, c_mem_size};
}

u8 read_byte_at(memory_access_t at, u32 offset)
{
    ASSERTF(offset < at.size, "Trying to read at off=%d beyond access size=%d", at.size, offset);
    u32 id = (at.base + offset) & c_mem_mask;
    return at.mem[id];
}

u16 read_word_at(memory_access_t at, u32 offset)
{
    return read_byte_at(at, offset) | (read_byte_at(at, offset+1) << 8);
}

void write_byte_to(memory_access_t to, u32 offset, u8 val)
{
    u32 id = (to.base + offset) & c_mem_mask;
    to.mem[id] = val;
}

void write_word_to(memory_access_t to, u32 offset, u16 val)
{
    write_byte_to(to, offset,   val & 0xFF);
    write_byte_to(to, offset+1, val >> 8);
}

u32 read_dword_at(memory_access_t at, u32 offset)
{
    return read_word_at(at, offset) | (read_word_at(at, offset+2) << 16);
}

void write_dword_to(memory_access_t to, u32 offset, u32 val)
{
    write_word_to(to, offset,   val & 0xFFFF);
    write_word_to(to, offset+2, val >> 16);
}

u32 get_full_address(memory_access_t at, u32 offset)
{
    return (at.base + offset) & c_mem_mask;
}

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

bool dump_memory_to_file(memory_access_t src, const char *fn)
{
    FILE *f = fopen(fn, "wb");
    if (!f)
        return false;

    size_t bytes_written = fwrite(src.mem + src.base, 1, src.size, f);

    fclose(f);
    return bytes_written == src.size;
}

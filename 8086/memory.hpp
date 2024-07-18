#pragma once
#include "defs.hpp"

struct memory_access_t {
    u8 *mem;
    u32 base;
    u32 size;
};

memory_access_t get_main_memory_access();

u8 read_byte_at(memory_access_t at, u32 offset);
u16 read_word_at(memory_access_t at, u32 offset);
u32 read_dword_at(memory_access_t at, u32 offset);
void write_byte_to(memory_access_t to, u32 offset, u8 val);
void write_word_to(memory_access_t to, u32 offset, u16 val);
void write_dword_to(memory_access_t to, u32 offset, u32 val);

u32 get_full_address(memory_access_t at, u32 offset);

u32 load_file_to_memory(memory_access_t dest, const char *fn);
bool dump_memory_to_file(memory_access_t src, const char *fn);

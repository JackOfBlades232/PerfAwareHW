/* decoder.cpp */
#define _CRT_SECURE_NO_WARNINGS
#include <cstdio>
#include <cstdint>
#include <cassert>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;

/* @TODO:
 * Create structs for instructions & state
 * Port table format from casey's repo
 * Build jump table
 * Write decoder
 * Make separate printing
 */

#define POT(_p) (1 << (_p))

#define ARR_CNT(_arr) (sizeof(_arr) / sizeof((_arr)[0]))

#define LOGERR(_fmt, ...) fprintf(stderr, "[ERR] " _fmt "\n", ##__VA_ARGS__)

typedef u8 memory_t[POT(20)];

enum reg_t {
    e_reg_a = 0,
    e_reg_b,
    e_reg_c,
    e_reg_d,

    e_reg_sp,
    e_reg_bp,
    e_reg_si,
    e_reg_di,

    e_reg_es,
    e_reg_cs,
    e_reg_ss,
    e_reg_ds,

    e_reg_ip,
    e_reg_flags,

    e_reg_max
};

enum instr_bits_t {
    e_bits_literal,

    e_bits_w,  
    e_bits_d,  
    e_bits_s,  
    e_bits_z,  
    e_bits_v,  

    e_bits_mod,
    e_bits_reg,
    e_bits_rm, 
    e_bits_sr, 

    e_bits_disp,
    e_bits_disp_always_w,
    e_bits_data,
    e_bits_data_w_if_w,
    e_bits_rm_always_w,
    e_bits_jmp_rel_disp,
    e_bits_far
};

enum op_t {
};

struct memory_access_t {
    u8 *mem;
    u32 base;
    u32 size;
};

struct instruction_t {
    op_t op;

    u32 size;
};

struct instruction_table_t {
};

struct decoder_context_t {

};

static memory_t g_memory;

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

instruction_t decode_next_instruction(memory_access_t at,
                                      instruction_table_t *table,
                                      decoder_context_t *ctx)
{
}

template <void (*t_insrunction_processor)(instruction_t)>
bool decode_and_process_instructions(memory_access_t code_mem, u32 code_bytes)
{
    instruction_table_t table = build_instruction_table();
    decoder_context_t ctx     = {};

    while (code_bytes)
    {
        instruction_t instr = decode_next_instruction(code_mem, &table, &ctx);

        if (code_bytes >= instr.size) {
            code_bytes    -= instr.size;
            code_mem.base += instr.size;
            code_mem.size -= instr.size;
        } else {
            LOGERR("Trying to decode outside the instructions mem, the instruction is invalid");
            return false;
        }

        update_ctx(instr, &ctx);
        t_insrunction_processor(instr);
    }
}

int main(int argc, char **argv)
{
    memory_access_t main_memory = {g_memory, 0, ARR_CNT(g_memory)};

    // @TODO: allow more files?
    // @TODO: sim by default, or with -s flag, disasm with -d flag
    if (argc != 2) {
        LOGERR("Invalid args, correct format: prog [file name]");
        return 1;
    }

    u32 code_bytes = load_file_to_memory(main_memory, argv[1]);

    return 0;
}

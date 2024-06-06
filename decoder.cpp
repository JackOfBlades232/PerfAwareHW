/* decoder.cpp */
#define _CRT_SECURE_NO_WARNINGS
#include <cstdint>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;

/* @TODO:
 * Create structs for instructions & state
 * Port table format from casey's repo
 * Build jump table -- CHECK
 * Write decoder
 * Make separate printing
 */

// @HUH: as for a simulation, I maybe should abstract memory acesses with looping and stuff

#define POT(_p) (1 << (_p))

#define ARR_CNT(_arr) (sizeof(_arr) / sizeof((_arr)[0]))

#define LOGERR(_fmt, ...) fprintf(stderr, "[ERR] " _fmt "\n", ##__VA_ARGS__)

inline u32 n_bit_mask(int nbits)
{
    return (1 << nbits) - 1;
}

inline int count_ones(u32 val, int bits_range)
{
    int res = 0;
    for (u32 mask = 1; mask != (1 << bits_range); mask <<= 1)
        res += (val & mask) ? 1 : 0;
    return res;
}

u32 reverse_bits_in_bytes(u32 val, int byte_range)
{
    u32 res = 0;
    int bits_range = byte_range * 8;
    u32 mask = 1;
    for (int i = 0; i < bits_range; ++i, mask <<= 1) {
        u32 src_mask = 1 << ((i/8 + 1)*8 - i%8 - 1);
        res |= (val & src_mask) ? mask : 0;
    }
    return res;
}

u32 press_down_masked_bits(u32 val, u32 mask)
{
    u32 res = 0;
    u32 read_mask = 1, write_mask = 1;
    for (int i = 0; i < 16; ++i, read_mask <<= 1) {
        if (mask & read_mask) {
            res |= (read_mask & val) ? write_mask : 0;
            write_mask <<= 1;
        }
    }

    return res;
}

u8 *alloc_pot(u32 p, size_t elem_size)
{
    assert(p <= 32);
    return (u8 *)malloc((1 << p) * elem_size);
}

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
    e_bits_end = 0, // denotes end of bit field list

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
    e_bits_far,

    e_bits_max
};

enum op_t {
    e_op_invalid = 0,

#define INST(_op, ...) e_op_##_op,
#define INSTALT(...)

#include "instruction_table.cpp.inl"
};

struct instr_bit_field_t {
    instr_bits_t type;
    u32 bit_count;
    u8 val;
};

struct instruction_encoding_t {
    op_t op;
    instr_bit_field_t fields[16] = {}; 
};

struct instruction_table_t {
    const instruction_encoding_t **table;
    u32 size;
    u16 mask; // we always check two bytes
};

static const instruction_encoding_t instruction_encoding_list[] = 
{
#include "instruction_table.cpp.inl"
};

instruction_table_t build_instruction_table(const instruction_encoding_t *enc_list, size_t enc_cnt)
{
    instruction_table_t table = {};
    for (const instruction_encoding_t *encp = enc_list; encp != enc_list + enc_cnt; ++encp) {
        int shift = 0;
        for (const instr_bit_field_t *f = encp->fields; f != encp->fields + 16; ++f) {
            if (f->type == e_bits_literal) // @TODO: check, this does promote to u32, doesnt it?
                table.mask |= n_bit_mask(f->bit_count) << shift;
            shift += f->bit_count;
        }
    }

    // @TODO: clean up the bit chaos here

    // Now bits are read as if they are lo -> hi, but in reality they are hi -> lo. So, reverse.
    // @SPEED: this is straightforwardly dumb.
    table.mask = reverse_bits_in_bytes(table.mask, 2);
    int address_bits = count_ones(table.mask, 16);

    table.size  = POT(address_bits);
    table.table = 
        (const instruction_encoding_t **)alloc_pot(address_bits, sizeof(instruction_encoding_t *));
    memset(table.table, 0, table.size * sizeof(instruction_encoding_t *));

    // @TODO: maybe I can merge the two loops?
    for (const instruction_encoding_t *encp = enc_list; encp != enc_list + enc_cnt; ++encp) {
        u16 lit_mask = 0, lit_vals = 0;
        int shift = 0;
        for (const instr_bit_field_t *f = encp->fields; f != encp->fields + 16; ++f) {
            if (f->type == e_bits_literal) { // @TODO: check, this does promote to u32, doesnt it?
                lit_mask |= n_bit_mask(f->bit_count) << shift;
                lit_vals |= (reverse_bits_in_bytes(f->val, 2) >> (8 - f->bit_count)) << shift;
            }
            shift += f->bit_count;
        }

        // Now bits are read as if they are lo -> hi, but in reality they are hi -> lo. So, reverse.
        // @SPEED: this is also straightforwardly dumb.
        lit_mask = reverse_bits_in_bytes(lit_mask, 2);
        lit_vals = reverse_bits_in_bytes(lit_vals, 2);

        // @SPEED: this may be done faster with something smarter than just a for loop on bits.
        //         However, does it matter?
        u16 id_mask = 0, id_val = 0;
        u16 read_mask = 1, write_mask = 1;
        int free_bit_cnt = 0;
        for (int i = 0; i < 16; ++i, read_mask <<= 1) {
            if (table.mask & read_mask) {
                if (lit_mask & read_mask) {
                    id_mask |= write_mask;
                    id_val  |= write_mask & lit_vals;
                } else
                    ++free_bit_cnt;
                write_mask <<= 1;
            }
        }

        // @TODO @SPEED: this is really lazy and suboptimal. Should be refactored for all purposes.
        for (u16 offset = 0; offset < 1 << free_bit_cnt; ++offset) {
            u16 bits = offset;
            u16 id = id_val;
            u16 mask = 1;
            for (int j = 0; j < 16; ++j, mask <<= 1) {
                if (!(id_mask & mask)) {
                    id |= (bits & 0b1) ? mask : 0;
                    bits >>= 1;
                }
            }

            table.table[id] = encp;
            //printf("%x: %lld\n", id, encp - enc_list);
        }
    }

    /*
    for (int i = 0; i < 2048; ++i) {
        if (table.table[i])
            printf("%x: %lld\n", i, table.table[i] - enc_list);
    }
    */

    return table;
}

struct memory_access_t {
    u8 *mem;
    u32 base;
    u32 size;
};

struct instruction_t {
    op_t op;

    u32 size;
};

struct decoder_context_t {

};

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
    u8 first_byte  = at.mem[at.base];
    u8 second_byte = at.mem[at.base+1];
    const instruction_encoding_t *enc =
        table->table[press_down_masked_bits(first_byte | (second_byte << 8), table->mask)];

    if (!enc) {
        LOGERR("Encountered unknown instruction, might be not implemented\n");
        return instruction_t{};
    }

    instruction_t instr = {};
    instr.op = enc->op;

    printf("op %d", instr.op);

    bool has[e_bits_max]  = {};
    u8 fields[e_bits_max] = {};

    u8 byte = first_byte;
    int bits_consumed = 0;
    for (const instr_bit_field_t *field = enc->fields; 
         field != enc->fields + 16 && field->type != e_bits_end;
         ++field)
    {
        has[field->type] = true;

        if (field->bit_count == 0) {
            fields[field->type] = field->val;

            printf(", %d:%x", field->type, fields[field->type]);
        } else {
            if (field->type != e_bits_literal) {
                fields[field->type] = byte >> (8 - field->bit_count);

                printf(", %d:%x", field->type, fields[field->type]);
            } else {
                assert(field->val == byte >> (8 - field->bit_count));
            }
            byte <<= field->bit_count;

            bits_consumed += field->bit_count;
            
            assert(bits_consumed <= 8);
            if (bits_consumed == 8) {
                byte = at.mem[++at.base];
                --at.size;

                ++instr.size;
                bits_consumed = 0;
            }
        }
    }

    assert(bits_consumed == 0);

    printf("\n");

    return instr;
}

void update_ctx(instruction_t instr, decoder_context_t *ctx)
{
}

void print_intstruction(instruction_t instr)
{
}

template <void (*t_insrunction_processor)(instruction_t)>
bool decode_and_process_instructions(memory_access_t at, u32 bytes)
{
    instruction_table_t table = build_instruction_table(instruction_encoding_list,
                                                        ARR_CNT(instruction_encoding_list));
    decoder_context_t ctx = {};

    while (bytes)
    {
        instruction_t instr = decode_next_instruction(at, &table, &ctx);
        if (instr.op == e_op_invalid) {
            LOGERR("Failed to decode instruction");
            return false;
        }

        if (bytes >= instr.size) {
            bytes   -= instr.size;
            at.base += instr.size;
            at.size -= instr.size;
        } else {
            LOGERR("Trying to decode outside the instructions mem, the instruction is invalid");
            return false;
        }

        update_ctx(instr, &ctx);
        t_insrunction_processor(instr);
    }

    return true;
}

static u8 g_memory[POT(20)];

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

    // @TEST
    bool res = decode_and_process_instructions<print_intstruction>(main_memory, code_bytes);
    return res ? 0 : 1;
}

/* decoder.cpp */
#include "defs.hpp"
#include "util.hpp"
#include "instruction.hpp"
#include "encoding.hpp"
#include "memory.hpp"
#include <cstdio>

/* @TODO:
 * Clean up for sim
 */

// @HUH: as for a simulation, I maybe should abstract memory acesses with looping and stuff

struct decoder_context_t {
    u32 last_pref;
    reg_t segment_override;
};

// @TODO: fix signed/unsigned business based on sign field. Also, for printing.
u16 parse_data_value(memory_access_t *at,
                     bool has_data, bool is_wide,
                     bool is_sign_extended = true)
{
    if (!has_data)
        return 0;

    u8 lo = at->mem[at->base++];
    if (is_wide) {
        u8 hi = at->mem[at->base++];
        return (hi << 8) | lo;
    } else if (is_sign_extended)
        return (i8)lo;
    else
        return lo;
}

operand_t get_reg_operand(u8 reg, bool is_wide)
{
    const reg_access_t reg_table[] =
    {
        {e_reg_a, 0, 1}, {e_reg_a,  0, 2}, 
        {e_reg_c, 0, 1}, {e_reg_c,  0, 2}, 
        {e_reg_d, 0, 1}, {e_reg_d,  0, 2}, 
        {e_reg_b, 0, 1}, {e_reg_b,  0, 2}, 
        {e_reg_a, 1, 1}, {e_reg_sp, 0, 2}, 
        {e_reg_c, 1, 1}, {e_reg_bp, 0, 2}, 
        {e_reg_d, 1, 1}, {e_reg_si, 0, 2}, 
        {e_reg_b, 1, 1}, {e_reg_di, 0, 2}
    };

    return {e_operand_reg, reg_table[((reg & 0x7) << 1) + (is_wide ? 1 : 0)]};
}

operand_t get_segreg_operand(u8 sr)
{
    const reg_access_t sr_table[] =
    {
        // @TODO: check order
        {e_reg_es, 0, 2},
        {e_reg_cs, 0, 2}, 
        {e_reg_ss, 0, 2},
        {e_reg_ds, 0, 2}, 
    };

    return {e_operand_reg, sr_table[sr & 0x3]};
}

operand_t get_rm_operand(u8 mod, u8 rm, bool is_wide, i16 disp)
{
    if (mod == 0b11)
        return get_reg_operand(rm, is_wide);

    if (mod == 0b00 && rm == 0b110)
        return {e_operand_mem, {.mem = {e_ea_base_direct, disp}}};

    const ea_base_t ea_base_table[] =
    {
        e_ea_base_bx_si,
        e_ea_base_bx_di,
        e_ea_base_bp_si,
        e_ea_base_bp_di,
        e_ea_base_si,
        e_ea_base_di,
        e_ea_base_bp,
        e_ea_base_bx
    };

    return {e_operand_mem, {.mem = {ea_base_table[rm & 0x7], disp}}};
}

operand_t get_imm_operand(u16 val)
{
    return {e_operand_imm, {.imm = val}};
}

operand_t get_cs_ip_operand(u16 disp, u16 data)
{
    return {e_operand_cs_ip, {.cs_ip = {data, disp}}};
}

instruction_t decode_next_instruction(memory_access_t at,
                                      instruction_table_t *table,
                                      const decoder_context_t *ctx)
{
    memory_access_t init_at = at;

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

    bool has[e_bits_max]  = {};
    u8 fields[e_bits_max] = {};

    u8 byte = first_byte;
    int bits_consumed = 0;
    for (const instr_bit_field_t *field = enc->fields; 
         field != enc->fields + ARR_CNT(enc->fields) && field->type != e_bits_end;
         ++field)
    {
        has[field->type] = true;

        if (field->bit_count == 0)
            fields[field->type] = field->val;
        else {
            if (field->type != e_bits_literal)
                fields[field->type] = byte >> (8 - field->bit_count);
            else
                assert(field->val == byte >> (8 - field->bit_count));
            byte <<= field->bit_count;

            bits_consumed += field->bit_count;
            
            assert(bits_consumed <= 8);
            if (bits_consumed == 8) {
                byte = at.mem[++at.base];
                --at.size;

                bits_consumed = 0;
            }
        }
    }

    assert(bits_consumed == 0);

    u8 mod = fields[e_bits_mod];
    u8 reg = fields[e_bits_reg];
    u8 rm  = fields[e_bits_rm];
    u8 w   = fields[e_bits_w];
    u8 s   = fields[e_bits_s];
    u8 d   = fields[e_bits_d];

    bool has_direct_address = (mod == 0b00 && rm == 0b110);
    has[e_bits_disp] = (has[e_bits_disp] || mod == 0b01 || mod == 0b10 || has_direct_address);

    bool disp_is_w = (mod == 0b10 || has_direct_address || fields[e_bits_disp_always_w]);
    bool data_is_w = (w && !s && fields[e_bits_data_w_if_w]);

    u16 disp = parse_data_value(&at, has[e_bits_disp], disp_is_w); 
    u16 data = parse_data_value(&at, has[e_bits_data], data_is_w, s); 

    operand_t *reg_op = &instr.operands[d ? 0 : 1];
    operand_t *rm_op  = &instr.operands[d ? 1 : 0];

    if (has[e_bits_reg])
        *reg_op = get_reg_operand(fields[e_bits_reg], w);
    if (has[e_bits_sr])
        *reg_op = get_segreg_operand(fields[e_bits_sr]);

    if (has[e_bits_rm])
        *rm_op = get_rm_operand(mod, rm, w || fields[e_bits_rm_always_w], disp);

    // @TODO: refactor this
    operand_t *last_op = &instr.operands[instr.operands[1].type ? 2 : (instr.operands[0].type ? 1 : 0)];
    operand_t *free_op = !instr.operands[0].type ? &instr.operands[0] :
                         !instr.operands[1].type ? &instr.operands[1] :
                         nullptr;

    if (free_op) {
        // @TODO: errors on more than one free_op
        if (has[e_bits_disp] && has[e_bits_data] && !has[e_bits_mod])
            *(free_op++) = get_cs_ip_operand(disp, data);
        else if (has[e_bits_data])
            *(free_op++) = get_imm_operand(data);
        else if (has[e_bits_v])
            *(free_op++) = fields[e_bits_v] ? get_reg_operand(0x1, false) : get_imm_operand(1);
        else if (fields[e_bits_jmp_rel_disp])
            *(free_op++) = get_imm_operand(disp);

        if (free_op > last_op)
            last_op = free_op;
    }

    if (w)
        instr.flags |= e_iflags_w;
    if (fields[e_bits_z])
        instr.flags |= e_iflags_z;
    instr.flags |= fields[e_bits_jmp_rel_disp] ? e_iflags_imm_is_rel_disp : 0;
    instr.flags |= fields[e_bits_far] ? e_iflags_far : 0;

    instr.flags |= ctx->last_pref;
    instr.segment_override = ctx->segment_override;

    instr.operand_cnt = last_op - instr.operands;
    instr.size        = at.base - init_at.base;

    return instr;
}

void update_ctx(instruction_t instr, decoder_context_t *ctx)
{
    if (instr.op == e_op_lock || instr.op == e_op_rep) {
        ctx->last_pref &= ~(e_iflags_lock | e_iflags_rep | e_iflags_z);
        if (instr.op == e_op_lock)
            ctx->last_pref |= e_iflags_lock;
        else if (instr.op == e_op_rep)
            ctx->last_pref |= e_iflags_rep | (instr.flags & e_iflags_z);
    }
    else if (instr.op == e_op_segment) {
        ctx->last_pref |= e_iflags_seg_override;
        ctx->segment_override = instr.operands[0].data.reg.reg;
    } else
      ctx->last_pref = 0;
}

void print_reg(reg_access_t reg)
{
    const char *reg_names[] =
    {
        "al", "ah", "ax",
        "bl", "bh", "bx",
        "cl", "ch", "cx",
        "dl", "dh", "dx",

        nullptr, nullptr, "sp",
        nullptr, nullptr, "bp",
        nullptr, nullptr, "si",
        nullptr, nullptr, "di",

        nullptr, nullptr, "es",
        nullptr, nullptr, "cs",
        nullptr, nullptr, "ss",
        nullptr, nullptr, "ds",
    };

    assert(reg.reg < e_reg_ip);

    // @NOTE: relying on there being three combinations: 01 02 11
    // @FEAT: will not do for 32bit
    int offset = reg.size == 2 ? 2 : (reg.offset == 1 ? 1 : 0);
    printf("%s", reg_names[reg.reg*3 + offset]);
}

void print_mem(ea_mem_access_t mem, bool override_seg, reg_t sr)
{
    const char *ea_base_names[e_ea_base_max] =
    {
        "bx+si", "bx+di", "bp+si", "bp+di",
        "si", "di", "bp", "bx",
    };

    const char *sr_names[] = { "es:", "cs:", "ss:", "ds:" };

    assert(mem.base < e_ea_base_max);
    const char *seg_override = override_seg ? sr_names[sr % 0x4] : "";
    if (mem.base == e_ea_base_direct)
        printf("%s[%hu]", seg_override, mem.disp);
    else if (mem.disp == 0)
        printf("%s[%s]", seg_override, ea_base_names[mem.base]);
    else
        printf("%s[%s%+hd]", seg_override, ea_base_names[mem.base], mem.disp);
}

void print_imm(u16 imm, bool is_rel_disp, u32 instr_size)
{
    if (is_rel_disp)
        printf("$%+hd+%d", imm, instr_size);
    else
        printf("%hd", imm);
}

void print_cs_ip(cs_ip_pair_t cs_ip)
{
    printf("%hu:%hu", cs_ip.cs, cs_ip.ip);
}

void print_intstruction(instruction_t instr)
{
    const char *op_mnemonics[e_op_max] =
    {
        "<invalid>",

#define INST(_op, ...) #_op,
#define INSTALT(...)

#include "instruction_table.cpp.inl"
    };

    // We print prefix as part of instruction
    if (instr.op == e_op_lock || instr.op == e_op_rep || instr.op == e_op_segment)
        return;

    if (instr.flags & e_iflags_lock)
        printf("lock ");
    if (instr.flags & e_iflags_rep)
        printf("rep%s ", (instr.flags & e_iflags_z) ? "" : "nz");

    printf("%s", op_mnemonics[instr.op]);

    if (instr.op == e_op_movs ||
        instr.op == e_op_cmps ||
        instr.op == e_op_scas ||
        instr.op == e_op_lods ||
        instr.op == e_op_stos)
    {
        printf("%c", (instr.flags & e_iflags_w) ? 'w' : 'b');
    }

    // For locked xchg, nasm wants reg to be second
    if (instr.op == e_op_xchg &&
        (instr.flags & e_iflags_lock) &&
        instr.operands[1].type != e_operand_reg)
    {
        swap(&instr.operands[0], &instr.operands[1]);
    }

    // @TODO: clean up
    if (instr.operand_cnt != 0) {
        printf(" ");

        for (int i = 0; i < instr.operand_cnt; ++i) {
            if (i == 1)
                printf(", ");

            operand_t *operand = &instr.operands[i];
            switch (operand->type) {
            case e_operand_none: break;

            case e_operand_reg:
                print_reg(operand->data.reg);
                break;
            case e_operand_mem:
                if (instr.flags & e_iflags_far)
                    printf("far ");
                // @NOTE: picked up from Casey's code. This produces not-so-neat
                //        prints for something like op [ea], reg, adding a redundant
                //        word/byte, but saves having to store more state in instructions
                else if (instr.operands[0].type != e_operand_reg)
                    printf((instr.flags & e_iflags_w) ? "word " : "byte ");
                print_mem(operand->data.mem, instr.flags & e_iflags_seg_override, instr.segment_override);
                break;
            case e_operand_imm:
                print_imm(operand->data.imm, instr.flags & e_iflags_imm_is_rel_disp, instr.size);
                break;
            case e_operand_cs_ip:
                print_cs_ip(operand->data.cs_ip);
                break;
            }
        }
    }

    printf("\n");
}

template <void (*t_insrunction_processor)(instruction_t)>
bool decode_and_process_instructions(memory_access_t at, u32 bytes)
{
    instruction_table_t table = build_instruction_table();
    decoder_context_t ctx = {};

    while (bytes) {
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
    printf(";; %s disassembly ;;\n", argv[1]);
    printf("bits 16\n");
    bool res = decode_and_process_instructions<print_intstruction>(main_memory, code_bytes);
    return res ? 0 : 1;
}

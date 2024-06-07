#include "instruction.hpp"
#include "util.hpp"
#include "decoder.hpp"

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
        const operand_t *first_free = free_op;
        if (has[e_bits_disp] && has[e_bits_data] && !has[e_bits_mod])
            *(free_op++) = get_cs_ip_operand(disp, data);
        else if (has[e_bits_data])
            *(free_op++) = get_imm_operand(data);
        else if (has[e_bits_v])
            *(free_op++) = fields[e_bits_v] ? get_reg_operand(0x1, false) : get_imm_operand(1);
        else if (fields[e_bits_jmp_rel_disp])
            *(free_op++) = get_imm_operand(disp);

        if (free_op - first_free > 1) {
            LOGERR("More than one free op processed, this is an error in instruction table");
            return instruction_t{};
        }

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

void update_decoder_ctx(instruction_t instr, decoder_context_t *ctx)
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

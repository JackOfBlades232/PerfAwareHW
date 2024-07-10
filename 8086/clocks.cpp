// #include "clocks.hpp"
#include "clocks.hpp"
#include "instruction.hpp"
#include "util.hpp"
#include <cassert>

// @TODO: add unaligned access penalty for 8086 (4 cycles per unaligned access)
//        and the same penalty for all on 8088

// @TODO: better digest for logs? More hidden patterns to decompose cycles?

static u32 estimate_ea_clocks(ea_mem_access_t ea)
{
    assert(ea.base < e_ea_base_max);

    if (ea.base == e_ea_base_direct)
        return 6;

    u32 cycles = 0;
    if (ea.base == e_ea_base_bp_di || ea.base == e_ea_base_bx_si)
        cycles += 7;
    else if (ea.base == e_ea_base_bp_si || ea.base == e_ea_base_bx_di)
        cycles += 8;
    else // bp/bx/si/di
        cycles += 5;

    if (ea.disp != 0)
        cycles += 4;
    
    return cycles;
}

static bool operand_is_seg_reg(operand_t op)
{
    return op.type == e_operand_reg &&
           (op.data.reg.reg == e_reg_cs || op.data.reg.reg == e_reg_ss ||
            op.data.reg.reg == e_reg_ds || op.data.reg.reg == e_reg_es);
}

static bool operand_is_flags_reg(operand_t op)
{
    return op.type == e_operand_reg && op.data.reg.reg == e_reg_flags;
}

static bool operands_are_acc_and_type(operand_t op0, operand_t op1, operand_type_t op1_req_type)
{
    if (op0.type != e_operand_reg || op0.data.reg.reg != e_reg_a)
        swap(&op0, &op1);
    return (op0.type == e_operand_reg && op0.data.reg.reg == e_reg_a &&
            op0.data.reg.offset == 0) &&
           op1.type == op1_req_type;
}

u32 estimate_instruction_clocks(instruction_metadata_t instr_data)
{
    // @TODO: additional cycles for prefixes

    instruction_t instr = instr_data.instr;

    const bool w   = instr.flags & e_iflags_w;
    const bool rep = instr.flags & e_iflags_rep;
    const bool far = instr.flags & e_iflags_far;

    int op_cnt    = instr.operand_cnt;
    operand_t op0 = instr.operands[0];
    operand_t op1 = instr.operands[1];

    switch (instr.op) {
    case e_op_mov:
        if (op0.type == e_operand_reg && op1.type == e_operand_reg)
            return 2;
        else if (op0.type == e_operand_reg && op1.type == e_operand_imm)
            return 4;
        // @TODO: check this, seems very sus
        else if (operands_are_acc_and_type(op0, op1, e_operand_mem))
            return 10;
        else if (op0.type == e_operand_reg && op1.type == e_operand_mem)
            return 8 + estimate_ea_clocks(op1.data.mem);
        else if (op0.type == e_operand_mem && op1.type == e_operand_reg)
            return 9 + estimate_ea_clocks(op0.data.mem);
        else // imm -> mem, should be validated by now
            return 10 + estimate_ea_clocks(op0.data.mem);

    case e_op_push:
    case e_op_pushf:
        if (operand_is_seg_reg(op0) || operand_is_flags_reg(op0))
            return 10;
        else if (op0.type == e_operand_reg)
            return 11;
        else // mem
            return 16 + estimate_ea_clocks(op0.data.mem);

    case e_op_pop:
    case e_op_popf:
        if (op0.type == e_operand_reg)
            return 8;
        else // mem
            return 17 + estimate_ea_clocks(op0.data.mem);

    case e_op_xchg:
        if (operands_are_acc_and_type(op0, op1, e_operand_reg) && w)
            return 3;
        else if (op0.type == e_operand_reg && op1.type == e_operand_reg)
            return 4;
        else // mem, reg
            return 17 + estimate_ea_clocks(op0.type == e_operand_mem ? op0.data.mem : op1.data.mem);

    case e_op_in:
    case e_op_out:
        if (op0.type == e_operand_reg && op1.type == e_operand_reg) // acc, dx
            return 8;
        else // acc, immed
            return 10;

    case e_op_xlat:
        return 11;

    case e_op_lea:
        return 2 + estimate_ea_clocks(op1.data.mem);

    case e_op_lds:
    case e_op_les:
        return 16 + estimate_ea_clocks(op1.data.mem);

    case e_op_lahf:
    case e_op_sahf:
    case e_op_aaa:
    case e_op_daa:
    case e_op_aas:
    case e_op_das:
        return 4;

    case e_op_add:
    case e_op_adc:
    case e_op_sub:
    case e_op_sbb:
    case e_op_and:
    case e_op_or:
    case e_op_xor:
        if (op0.type == e_operand_reg && op1.type == e_operand_reg)
            return 3;
        else if (op0.type == e_operand_reg && op1.type == e_operand_imm)
            return 4;
        else if (op0.type == e_operand_reg && op1.type == e_operand_mem)
            return 9 + estimate_ea_clocks(op1.data.mem);
        else if (op0.type == e_operand_mem && op1.type == e_operand_reg)
            return 16 + estimate_ea_clocks(op0.data.mem);
        else // imm -> mem, should be validated by now
            return 17 + estimate_ea_clocks(op0.data.mem);

    case e_op_cmp:
        if (op0.type == e_operand_reg && op1.type == e_operand_reg)
            return 3;
        else if (op0.type == e_operand_reg && op1.type == e_operand_imm)
            return 4;
        else if (op0.type == e_operand_reg && op1.type == e_operand_mem)
            return 9 + estimate_ea_clocks(op1.data.mem);
        else if (op0.type == e_operand_mem && op1.type == e_operand_reg)
            return 9 + estimate_ea_clocks(op0.data.mem);
        else // imm -> mem, should be validated by now
            return 10 + estimate_ea_clocks(op0.data.mem);

    // @TODO: these are ranged, depict somehow? Now, I am using high estimate
    case e_op_mul:
        if (op0.type == e_operand_reg && !w)
            return 77;
        else if (op0.type == e_operand_reg && w)
            return 133;
        else if (op0.type == e_operand_mem && !w)
            return 83 + estimate_ea_clocks(op0.data.mem);
        else if (op0.type == e_operand_mem && w)
            return 139 + estimate_ea_clocks(op0.data.mem);

    case e_op_imul:
        if (op0.type == e_operand_reg && !w)
            return 98;
        else if (op0.type == e_operand_reg && w)
            return 154;
        else if (op0.type == e_operand_mem && !w)
            return 104 + estimate_ea_clocks(op0.data.mem);
        else if (op0.type == e_operand_mem && w)
            return 160 + estimate_ea_clocks(op0.data.mem);

    case e_op_aam:
        return 83;

    // @TODO: these are ranged, depict somehow? Now, I am using high estimate
    case e_op_div:
        if (op0.type == e_operand_reg && !w)
            return 90;
        else if (op0.type == e_operand_reg && w)
            return 162;
        else if (op0.type == e_operand_mem && !w)
            return 96 + estimate_ea_clocks(op0.data.mem);
        else if (op0.type == e_operand_mem && w)
            return 168 + estimate_ea_clocks(op0.data.mem);

    case e_op_idiv:
        if (op0.type == e_operand_reg && !w)
            return 112;
        else if (op0.type == e_operand_reg && w)
            return 184;
        else if (op0.type == e_operand_mem && !w)
            return 118 + estimate_ea_clocks(op0.data.mem);
        else if (op0.type == e_operand_mem && w)
            return 190 + estimate_ea_clocks(op0.data.mem);

    case e_op_aad:
        return 60;

    case e_op_cbw:
        return 2;
    case e_op_cwd:
        return 5;

    case e_op_test:
        if (op0.type == e_operand_reg && op1.type == e_operand_reg)
            return 3;
        // @TODO: check this, seems sus
        else if (operands_are_acc_and_type(op0, op1, e_operand_imm))
            return 4;
        else if (op0.type == e_operand_reg && op1.type == e_operand_imm)
            return 5;
        else if (op0.type == e_operand_reg && op1.type == e_operand_mem)
            return 9 + estimate_ea_clocks(op1.data.mem);
        else // imm & mem, should be validated by now
            return 11 + estimate_ea_clocks(op0.data.mem);

    case e_op_inc:
    case e_op_dec:
        if (op0.type == e_operand_reg)
            return 4 - op0.data.reg.size; // 2 for 16bit, 3 for 8bit
        else // mem
            return 15 + estimate_ea_clocks(op0.data.mem);

    case e_op_neg:
        if (op0.type == e_operand_reg)
            return 4;
        else // mem
            return 16 + estimate_ea_clocks(op0.data.mem);

    case e_op_not:
        if (op0.type == e_operand_reg)
            return 3;
        else // mem
            return 16 + estimate_ea_clocks(op0.data.mem);

    case e_op_shl:
    case e_op_shr:
    case e_op_sar:
    case e_op_rol:
    case e_op_ror:
    case e_op_rcl:
    case e_op_rcr:
        if (op0.type == e_operand_reg && op1.type == e_operand_imm) // must be reg, 1
            return 2;
        else if (op0.type == e_operand_reg && op1.type == e_operand_reg) // must be reg, cl
            return 8 + 4*instr_data.op1_val;
        if (op0.type == e_operand_mem && op1.type == e_operand_imm) // must be mem, 1
            return 15 + estimate_ea_clocks(op0.data.mem);
        else // must be mem, cl
            return 20 + estimate_ea_clocks(op0.data.mem) + 4*instr_data.op1_val;

    case e_op_movs:
        if (rep)
            return 9 + 17*instr_data.rep_count;
        else
            return 18;

    case e_op_cmps:
        return (rep ? 9 : 0) + 22*instr_data.rep_count;
    case e_op_scas:
        return (rep ? 9 : 0) + 15*instr_data.rep_count;

    case e_op_lods:
        if (rep)
            return 9 + 13*instr_data.rep_count;
        else
            return 12;

    case e_op_stos:
        if (rep)
            return 9 + 10*instr_data.rep_count;
        else
            return 11;

    case e_op_call:
        if (op0.type == e_operand_reg)
            return 16;
        else if (op0.type == e_operand_imm)
            return 19;
        else if (op0.type == e_operand_cs_ip)
            return 28;
        else if (op0.type == e_operand_mem)
            return far ? 37 : 21 + estimate_ea_clocks(op0.data.mem);

    case e_op_jmp:
        if (op0.type == e_operand_reg)
            return 11;
        else if (op0.type == e_operand_imm || op0.type == e_operand_cs_ip)
            return 15; // @TODO: this is sus
        else if (op0.type == e_operand_mem)
            return far ? 24 : 18 + estimate_ea_clocks(op0.data.mem);

    case e_op_je:
    case e_op_jl:
    case e_op_jle:
    case e_op_jb:
    case e_op_jbe:
    case e_op_jp:
    case e_op_jo:
    case e_op_js:
    case e_op_jne:
    case e_op_jge:
    case e_op_jg:
    case e_op_jae:
    case e_op_ja:
    case e_op_jnp:
    case e_op_jno:
    case e_op_jns:
        return instr_data.cond_action_happened ? 16 : 4;

    case e_op_loop:
        return instr_data.cond_action_happened ? 17 : 5;
    case e_op_loopnz:
        return instr_data.cond_action_happened ? 19 : 5;
    case e_op_loopz:
    case e_op_jcxz:
        return instr_data.cond_action_happened ? 18 : 6;

    case e_op_lock:
    case e_op_rep:
    case e_op_segment:
        LOGERR("Don't query cycle counts on raw prefixes");
        // Fallthrough to assert(0)

    default: LOGERR("Cycle count not implemented"); assert(0);
    }

    return 0;
}

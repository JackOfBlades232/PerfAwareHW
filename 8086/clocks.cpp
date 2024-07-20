// #include "clocks.hpp"
#include "clocks.hpp"
#include "instruction.hpp"
#include "util.hpp"
#include <cassert>

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

u32 estimate_instruction_clocks(instruction_metadata_t instr_data, proc_type_t proc_type)
{
    instruction_t instr = instr_data.instr;

    const bool w   = instr.flags & e_iflags_w;
    const bool far = instr.flags & e_iflags_far;

    const bool rep          = instr.flags & e_iflags_rep;
    const bool lock         = instr.flags & e_iflags_lock;
    const bool seg_override = instr.flags & e_iflags_seg_override;

    u32 clocks = 0;

    if (rep)
        clocks += 2;
    if (lock)
        clocks += 2;
    if (seg_override)
        clocks += 2;
    
    switch (proc_type) {
    case e_proc8086:
        clocks += 4 * instr_data.wide_odd_transfer_cnt;
        break;
    case e_proc8088:
        clocks += 4 * instr_data.wide_transfer_cnt;
        break;
    }

    int op_cnt    = instr.operand_cnt;
    operand_t op0 = instr.operands[0];
    operand_t op1 = instr.operands[1];

    switch (instr.op) {
    #define CASE_ADD_CLOCKS(clocks_) { clocks += (clocks_); break; }
    case e_op_mov:
        if (op0.type == e_operand_reg && op1.type == e_operand_reg)
            CASE_ADD_CLOCKS(2)
        else if (op0.type == e_operand_reg && op1.type == e_operand_imm)
            CASE_ADD_CLOCKS(4)
        // @TODO: check this, seems very sus
        else if (operands_are_acc_and_type(op0, op1, e_operand_mem))
            CASE_ADD_CLOCKS(10)
        else if (op0.type == e_operand_reg && op1.type == e_operand_mem)
            CASE_ADD_CLOCKS(8 + estimate_ea_clocks(op1.data.mem))
        else if (op0.type == e_operand_mem && op1.type == e_operand_reg)
            CASE_ADD_CLOCKS(9 + estimate_ea_clocks(op0.data.mem))
        else // imm -> mem, should be validated by now
            CASE_ADD_CLOCKS(10 + estimate_ea_clocks(op0.data.mem))

    case e_op_push:
    case e_op_pushf:
        if (operand_is_seg_reg(op0) || operand_is_flags_reg(op0))
            CASE_ADD_CLOCKS(10)
        else if (op0.type == e_operand_reg)
            CASE_ADD_CLOCKS(11)
        else // mem
            CASE_ADD_CLOCKS(16 + estimate_ea_clocks(op0.data.mem))

    case e_op_pop:
    case e_op_popf:
        if (op0.type == e_operand_reg)
            CASE_ADD_CLOCKS(8)
        else // mem
            CASE_ADD_CLOCKS(17 + estimate_ea_clocks(op0.data.mem))

    case e_op_xchg:
        if (operands_are_acc_and_type(op0, op1, e_operand_reg) && w)
            CASE_ADD_CLOCKS(3)
        else if (op0.type == e_operand_reg && op1.type == e_operand_reg)
            CASE_ADD_CLOCKS(4)
        else // mem, reg
            CASE_ADD_CLOCKS(17 + estimate_ea_clocks(op0.type == e_operand_mem ? op0.data.mem : op1.data.mem))

    case e_op_in:
    case e_op_out:
        if (op0.type == e_operand_reg && op1.type == e_operand_reg) // acc, dx
            CASE_ADD_CLOCKS(8)
        else // acc, immed
            CASE_ADD_CLOCKS(10)

    case e_op_xlat:
        CASE_ADD_CLOCKS(11)

    case e_op_lea:
        CASE_ADD_CLOCKS(2 + estimate_ea_clocks(op1.data.mem))

    case e_op_lds:
    case e_op_les:
        CASE_ADD_CLOCKS(16 + estimate_ea_clocks(op1.data.mem))

    case e_op_lahf:
    case e_op_sahf:
    case e_op_aaa:
    case e_op_daa:
    case e_op_aas:
    case e_op_das:
        CASE_ADD_CLOCKS(4)

    case e_op_add:
    case e_op_adc:
    case e_op_sub:
    case e_op_sbb:
    case e_op_and:
    case e_op_or:
    case e_op_xor:
        if (op0.type == e_operand_reg && op1.type == e_operand_reg)
            CASE_ADD_CLOCKS(3)
        else if (op0.type == e_operand_reg && op1.type == e_operand_imm)
            CASE_ADD_CLOCKS(4)
        else if (op0.type == e_operand_reg && op1.type == e_operand_mem)
            CASE_ADD_CLOCKS(9 + estimate_ea_clocks(op1.data.mem))
        else if (op0.type == e_operand_mem && op1.type == e_operand_reg)
            CASE_ADD_CLOCKS(16 + estimate_ea_clocks(op0.data.mem))
        else // imm -> mem, should be validated by now
            CASE_ADD_CLOCKS(17 + estimate_ea_clocks(op0.data.mem))

    case e_op_cmp:
        if (op0.type == e_operand_reg && op1.type == e_operand_reg)
            CASE_ADD_CLOCKS(3)
        else if (op0.type == e_operand_reg && op1.type == e_operand_imm)
            CASE_ADD_CLOCKS(4)
        else if (op0.type == e_operand_reg && op1.type == e_operand_mem)
            CASE_ADD_CLOCKS(9 + estimate_ea_clocks(op1.data.mem))
        else if (op0.type == e_operand_mem && op1.type == e_operand_reg)
            CASE_ADD_CLOCKS(9 + estimate_ea_clocks(op0.data.mem))
        else // imm -> mem, should be validated by now
            CASE_ADD_CLOCKS(10 + estimate_ea_clocks(op0.data.mem))

    // @TODO: these are ranged, depict somehow? Now, I am using high estimate
    case e_op_mul:
        if (op0.type == e_operand_reg && !w)
            CASE_ADD_CLOCKS(77)
        else if (op0.type == e_operand_reg && w)
            CASE_ADD_CLOCKS(133)
        else if (op0.type == e_operand_mem && !w)
            CASE_ADD_CLOCKS(83 + estimate_ea_clocks(op0.data.mem))
        else if (op0.type == e_operand_mem && w)
            CASE_ADD_CLOCKS(139 + estimate_ea_clocks(op0.data.mem))

    case e_op_imul:
        if (op0.type == e_operand_reg && !w)
            CASE_ADD_CLOCKS(98)
        else if (op0.type == e_operand_reg && w)
            CASE_ADD_CLOCKS(154)
        else if (op0.type == e_operand_mem && !w)
            CASE_ADD_CLOCKS(104 + estimate_ea_clocks(op0.data.mem))
        else if (op0.type == e_operand_mem && w)
            CASE_ADD_CLOCKS(160 + estimate_ea_clocks(op0.data.mem))

    case e_op_aam:
        CASE_ADD_CLOCKS(83)

    // @NOTE: these are ranged, I am using the high estimate.
    case e_op_div:
        if (op0.type == e_operand_reg && !w)
            CASE_ADD_CLOCKS(90)
        else if (op0.type == e_operand_reg && w)
            CASE_ADD_CLOCKS(162)
        else if (op0.type == e_operand_mem && !w)
            CASE_ADD_CLOCKS(96 + estimate_ea_clocks(op0.data.mem))
        else if (op0.type == e_operand_mem && w)
            CASE_ADD_CLOCKS(168 + estimate_ea_clocks(op0.data.mem))

    case e_op_idiv:
        if (op0.type == e_operand_reg && !w)
            CASE_ADD_CLOCKS(112)
        else if (op0.type == e_operand_reg && w)
            CASE_ADD_CLOCKS(184)
        else if (op0.type == e_operand_mem && !w)
            CASE_ADD_CLOCKS(118 + estimate_ea_clocks(op0.data.mem))
        else if (op0.type == e_operand_mem && w)
            CASE_ADD_CLOCKS(190 + estimate_ea_clocks(op0.data.mem))

    case e_op_aad:
        CASE_ADD_CLOCKS(60)

    case e_op_cbw:
    case e_op_clc:
    case e_op_stc:
    case e_op_cmc:
    case e_op_cld:
    case e_op_std:
    case e_op_cli:
    case e_op_sti:
    case e_op_hlt:
        CASE_ADD_CLOCKS(2)
    case e_op_cwd:
        CASE_ADD_CLOCKS(5)

    case e_op_test:
        if (op0.type == e_operand_reg && op1.type == e_operand_reg)
            CASE_ADD_CLOCKS(3)
        // @TODO: check this, seems sus
        else if (operands_are_acc_and_type(op0, op1, e_operand_imm))
            CASE_ADD_CLOCKS(4)
        else if (op0.type == e_operand_reg && op1.type == e_operand_imm)
            CASE_ADD_CLOCKS(5)
        else if (op0.type == e_operand_reg && op1.type == e_operand_mem)
            CASE_ADD_CLOCKS(9 + estimate_ea_clocks(op1.data.mem))
        else // imm & mem, should be validated by now
            CASE_ADD_CLOCKS(11 + estimate_ea_clocks(op0.data.mem))

    case e_op_inc:
    case e_op_dec:
        if (op0.type == e_operand_reg)
            CASE_ADD_CLOCKS(4 - op0.data.reg.size) // 2 for 16bit, 3 for 8bit
        else // mem
            CASE_ADD_CLOCKS(15 + estimate_ea_clocks(op0.data.mem))

    case e_op_neg:
        if (op0.type == e_operand_reg)
            CASE_ADD_CLOCKS(4)
        else // mem
            CASE_ADD_CLOCKS(16 + estimate_ea_clocks(op0.data.mem))

    case e_op_not:
        if (op0.type == e_operand_reg)
            CASE_ADD_CLOCKS(3)
        else // mem
            CASE_ADD_CLOCKS(16 + estimate_ea_clocks(op0.data.mem))

    case e_op_shl:
    case e_op_shr:
    case e_op_sar:
    case e_op_rol:
    case e_op_ror:
    case e_op_rcl:
    case e_op_rcr:
        if (op0.type == e_operand_reg && op1.type == e_operand_imm) // must be reg, 1
            CASE_ADD_CLOCKS(2)
        else if (op0.type == e_operand_reg && op1.type == e_operand_reg) // must be reg, cl
            CASE_ADD_CLOCKS(8 + 4*instr_data.op1_val)
        if (op0.type == e_operand_mem && op1.type == e_operand_imm) // must be mem, 1
            CASE_ADD_CLOCKS(15 + estimate_ea_clocks(op0.data.mem))
        else // must be mem, cl
            CASE_ADD_CLOCKS(20 + estimate_ea_clocks(op0.data.mem) + 4*instr_data.op1_val)

    case e_op_movs:
        if (rep)
            CASE_ADD_CLOCKS(9 + 17*instr_data.rep_count)
        else
            CASE_ADD_CLOCKS(18)

    case e_op_cmps:
        CASE_ADD_CLOCKS((rep ? 9 : 0) + 22*instr_data.rep_count)
    case e_op_scas:
        CASE_ADD_CLOCKS((rep ? 9 : 0) + 15*instr_data.rep_count)

    case e_op_lods:
        if (rep)
            CASE_ADD_CLOCKS(9 + 13*instr_data.rep_count)
        else
            CASE_ADD_CLOCKS(12)

    case e_op_stos:
        if (rep)
            CASE_ADD_CLOCKS(9 + 10*instr_data.rep_count)
        else
            CASE_ADD_CLOCKS(11)

    case e_op_call:
        if (op0.type == e_operand_reg)
            CASE_ADD_CLOCKS(16)
        else if (op0.type == e_operand_imm)
            CASE_ADD_CLOCKS(19)
        else if (op0.type == e_operand_cs_ip)
            CASE_ADD_CLOCKS(28)
        else if (op0.type == e_operand_mem)
            CASE_ADD_CLOCKS(far ? 37 : 21 + estimate_ea_clocks(op0.data.mem))

    case e_op_jmp:
        if (op0.type == e_operand_reg)
            CASE_ADD_CLOCKS(11)
        else if (op0.type == e_operand_imm || op0.type == e_operand_cs_ip)
            CASE_ADD_CLOCKS(15) // @TODO: this is sus
        else if (op0.type == e_operand_mem)
            CASE_ADD_CLOCKS(far ? 24 : 18 + estimate_ea_clocks(op0.data.mem))

    case e_op_ret:
        CASE_ADD_CLOCKS(op_cnt ? 12 : 8)
    case e_op_retf:
        CASE_ADD_CLOCKS(op_cnt ? 17 : 18) // @TODO: seems like a typo

    case e_op_int:
        CASE_ADD_CLOCKS(51)
    case e_op_int3:
        CASE_ADD_CLOCKS(52)

    case e_op_into:
        CASE_ADD_CLOCKS(instr_data.cond_action_happened ? 53 : 4)

    case e_op_iret:
        CASE_ADD_CLOCKS(24)

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
        CASE_ADD_CLOCKS(instr_data.cond_action_happened ? 16 : 4)

    case e_op_loop:
        CASE_ADD_CLOCKS(instr_data.cond_action_happened ? 17 : 5)
    case e_op_loopnz:
        CASE_ADD_CLOCKS(instr_data.cond_action_happened ? 19 : 5)
    case e_op_loopz:
    case e_op_jcxz:
        CASE_ADD_CLOCKS(instr_data.cond_action_happened ? 18 : 6)

    case e_op_wait:
        CASE_ADD_CLOCKS(3 + 5*instr_data.wait_n)

    case e_op_nop:
        CASE_ADD_CLOCKS(3)

    case e_op_lock:
    case e_op_rep:
    case e_op_segment:
        LOGERR("Don't query cycle counts on raw prefixes");
        // Fallthrough to ASSERTF(0)

    default: ASSERTF(0, "Cycle count not implemented");

    #undef CASE_ADD_CLOCKS
    }

    return clocks;
}

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

static bool operands_are_acc_non_reg(operand_t op0, operand_t op1, operand_type_t op2_type)
{
    if (op0.type != e_operand_reg)
        swap(&op0, &op1);
    return (op0.type == e_operand_reg && op0.data.reg.reg == e_reg_a &&
            op0.data.reg.offset == 0) &&
           op1.type == op2_type;
}

u32 estimate_instruction_clocks(instruction_metadata_t instr_data)
{
    // @TODO: additional cycles for prefixes

    instruction_t instr = instr_data.instr;

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
        else if (operands_are_acc_non_reg(op0, op1, e_operand_mem))
            return 10;
        else if (op0.type == e_operand_reg && op1.type == e_operand_mem)
            return 8 + estimate_ea_clocks(op1.data.mem);
        else if (op0.type == e_operand_mem && op1.type == e_operand_reg)
            return 9 + estimate_ea_clocks(op0.data.mem);
        else // imm -> mem, should be validated by now
            return 10 + estimate_ea_clocks(op0.data.mem);

    case e_op_add:
    case e_op_sub:
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

    case e_op_test:
        if (op0.type == e_operand_reg && op1.type == e_operand_reg)
            return 3;
        // @TODO: check this, seems sus
        else if (operands_are_acc_non_reg(op0, op1, e_operand_imm))
            return 4;
        else if (op0.type == e_operand_reg && op1.type == e_operand_imm)
            return 5;
        else if (op0.type == e_operand_reg && op1.type == e_operand_mem)
            return 9 + estimate_ea_clocks(op1.data.mem);
        else // imm & mem, should be validated by now
            return 11 + estimate_ea_clocks(op0.data.mem);

    case e_op_inc:
    case e_op_dec:
        if (op0.type == e_operand_reg) {
            if (op0.data.reg.size == 2)
                return 2;
            else
                return 3;
        } else // mem
            return 15 + estimate_ea_clocks(op0.data.mem);

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


    default: LOGERR("Cycle count not implemented"); assert(0);
    }

    return 0;
}

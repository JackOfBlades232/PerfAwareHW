#include "validation.hpp"
#include "instruction.hpp"
#include <cassert>

#define VERIFY(e_) do { if (!(e_)) return false; } while (0)

bool validate_instruction(instruction_t instr)
{
    VERIFY(instr.op != e_op_invalid);
    VERIFY(instr.op != e_op_max);

    VERIFY(instr.operand_cnt <= 2);
    for (int i = 0; i < instr.operand_cnt; ++i) {
        VERIFY(instr.operands[i].type != e_operand_none);
        if (instr.operands[i].type == e_operand_mem)
            VERIFY(instr.operands[i].data.mem.base != e_ea_base_max);
        else if (instr.operands[i].type == e_operand_reg) {
            VERIFY(instr.operands[i].data.reg.reg != e_reg_max);
            VERIFY(instr.operands[i].data.reg.size == 2 ||
                   (instr.operands[i].data.reg.size == 1 &&
                    instr.operands[i].data.reg.offset <= 1));
        }
    }
    for (int i = instr.operand_cnt; i < 2; ++i)
        VERIFY(instr.operands[i].type == e_operand_none);

    if (instr.flags & e_iflags_seg_override) 
        VERIFY(instr.segment_override == e_reg_ds || instr.segment_override == e_reg_ss || instr.segment_override == e_reg_es);
    else
        VERIFY(instr.segment_override == e_reg_max);

    const u32 op_cnt = instr.operand_cnt;
    const operand_t op0 = instr.operands[0];
    const operand_t op1 = instr.operands[1];

    const u32 flags = instr.flags;

    const bool w        = flags & e_iflags_w;
    const bool s        = flags & e_iflags_s;
    const bool z        = flags & e_iflags_z;
    const bool rep      = flags & e_iflags_rep;
    const bool lock     = flags & e_iflags_lock;
    const bool rel_disp = flags & e_iflags_imm_is_rel_disp;
    const bool far      = flags & e_iflags_far;

    switch (instr.op) {
    case e_op_mov:
        VERIFY(op_cnt == 2);
        VERIFY(op0.type == e_operand_mem || op0.type == e_operand_reg);
        VERIFY(op1.type != e_operand_cs_ip);
        VERIFY(!s && !z && !rep && !rel_disp && !far);
        break;

    case e_op_push:
        VERIFY(op_cnt == 1);
        VERIFY(op0.type != e_operand_cs_ip);
        VERIFY(w);
        VERIFY(!s && !z && !rep && !rel_disp && !far);
        break;

    case e_op_pop:
        VERIFY(op_cnt == 1);
        VERIFY(op0.type == e_operand_mem || op0.type == e_operand_reg);
        VERIFY(w);
        VERIFY(!s && !z && !rep && !rel_disp && !far);
        break;

    case e_op_xchg:
        VERIFY(op_cnt == 2);
        VERIFY(op0.type == e_operand_mem || op0.type == e_operand_reg);
        VERIFY(op1.type == e_operand_mem || op1.type == e_operand_reg);
        VERIFY(!s && !z && !rep && !rel_disp && !far);
        break;


    case e_op_lea:
    case e_op_lds:
    case e_op_les:
        VERIFY(op_cnt == 2);
        VERIFY(op0.type == e_operand_mem || op0.type == e_operand_reg);
        VERIFY(op1.type == e_operand_mem);
        VERIFY(flags == 0);
        break;

    case e_op_adc:
    case e_op_add:
    case e_op_sbb:
    case e_op_sub:
    case e_op_cmp:
    case e_op_and:
    case e_op_test:
    case e_op_or:
    case e_op_xor:
        VERIFY(op_cnt == 2);
        VERIFY(op0.type == e_operand_mem || op0.type == e_operand_reg);
        VERIFY(op1.type != e_operand_cs_ip);
        VERIFY(!z && !rep && !rel_disp && !far);
        break;

    case e_op_mul:
    case e_op_imul:
    case e_op_div:
    case e_op_idiv:
        VERIFY(op_cnt == 1);
        VERIFY(op0.type != e_operand_cs_ip);
        VERIFY(!s && !z && !rep && !rel_disp && !far);
        break;

    case e_op_inc:
    case e_op_dec:
    case e_op_neg:
    case e_op_not:
        VERIFY(op_cnt == 1);
        VERIFY(op0.type == e_operand_mem || op0.type == e_operand_reg);
        VERIFY(!s && !z && !rep && !rel_disp && !far);
        break;

    case e_op_shl:
    case e_op_shr:
    case e_op_sar:
    case e_op_rol:
    case e_op_ror:
    case e_op_rcl:
    case e_op_rcr:
        VERIFY(op_cnt == 2);
        VERIFY(op0.type == e_operand_mem || op0.type == e_operand_reg);
        VERIFY(op1.type == e_operand_imm || op1.type == e_operand_reg);
        if (op1.type == e_operand_imm)
            VERIFY(op1.data.imm == 1);
        else
            VERIFY(op1.data.reg.reg == e_reg_c && op1.data.reg.offset == 0 && op1.data.reg.size == 1);
        VERIFY(!s && !z && !rep && !rel_disp && !far);
        break;

    case e_op_movs:
    case e_op_cmps:
    case e_op_scas:
    case e_op_lods:
    case e_op_stos:
        VERIFY(op_cnt == 0);
        VERIFY(!s && !z && !rel_disp && !far);
        break;

    case e_op_call:
    case e_op_jmp:
        VERIFY(op_cnt == 1);
        VERIFY(w || instr.op == e_op_jmp);
        if (op0.type == e_operand_imm)
            VERIFY(rel_disp);

        VERIFY(!s && !z && !rep);
        break;

    case e_op_ret:
    case e_op_retf:
        VERIFY(op_cnt <= 1);
        if (op_cnt)
            VERIFY(op0.type == e_operand_imm);
        break;

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
    case e_op_loop:
    case e_op_loopz:
    case e_op_loopnz:
    case e_op_jcxz:
    case e_op_int:
        VERIFY(op_cnt == 1);
        VERIFY(op0.type == e_operand_imm);
        VERIFY(flags == 0);
        break;

    case e_op_in:
        VERIFY(op_cnt == 2);
        VERIFY(op0.type == e_operand_reg && op0.data.reg.reg == e_reg_a);
        VERIFY(op1.type == e_operand_imm);
        VERIFY(!s && !z && !rep && !rel_disp && !far);
        break;
        
    case e_op_out:
        VERIFY(op_cnt == 2);
        VERIFY(op0.type == e_operand_imm);
        VERIFY(op1.type == e_operand_reg && op1.data.reg.reg == e_reg_a);
        VERIFY(!s && !z && !rep && !rel_disp && !far);
        break;

    case e_op_esc:
        VERIFY(op_cnt == 2);
        VERIFY(op0.type == e_operand_imm && op0.data.imm < (1 << 6));
        VERIFY(op1.type == e_operand_reg && op1.type == e_operand_mem);
        break;

    case e_op_xlat:
    case e_op_lahf:
    case e_op_sahf:
    case e_op_pushf:
    case e_op_popf:
    case e_op_aaa:
    case e_op_aas:
    case e_op_daa:
    case e_op_das:
    case e_op_aam:
    case e_op_aad:
    case e_op_cbw:
    case e_op_cwd:
    case e_op_clc:
    case e_op_stc:
    case e_op_cmc:
    case e_op_cld:
    case e_op_std:
    case e_op_cli:
    case e_op_sti:
    case e_op_nop:
    case e_op_into:
    case e_op_iret:
    case e_op_int3:
    case e_op_hlt:
    case e_op_wait:
        VERIFY(op_cnt == 0);
        VERIFY(flags == 0);
        break;

    default:
        VERIFY(0);
    }

    return true;
}

bool validate_instruction_metadata(instruction_metadata_t instr_data)
{
    assert(validate_instruction(instr_data.instr));
    const bool w        = instr_data.instr.flags & e_iflags_w;
    const bool s        = instr_data.instr.flags & e_iflags_s;
    const bool z        = instr_data.instr.flags & e_iflags_z;
    const bool rep      = instr_data.instr.flags & e_iflags_rep;
    const bool lock     = instr_data.instr.flags & e_iflags_lock;
    const bool rel_disp = instr_data.instr.flags & e_iflags_imm_is_rel_disp;
    const bool far      = instr_data.instr.flags & e_iflags_far;

    if (instr_data.instr.operands[0].type != e_operand_cs_ip)
        VERIFY(instr_data.op0_val < (1 << (w ? 16 : 8)));
    if (instr_data.instr.operands[1].type != e_operand_cs_ip)
        VERIFY(instr_data.op1_val < (1 << (w ? 16 : 8)));

    switch (instr_data.instr.op) {
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
    case e_op_loop:
    case e_op_loopz:
    case e_op_loopnz:
    case e_op_jcxz:
    case e_op_into:
        VERIFY(instr_data.rep_count == 0);
        VERIFY(instr_data.wait_n == 0);
        break;

    case e_op_movs:
    case e_op_cmps:
    case e_op_scas:
    case e_op_lods:
    case e_op_stos:
        VERIFY(!instr_data.cond_action_happened);
        VERIFY(instr_data.rep_count > 0);
        VERIFY(instr_data.wait_n == 0);
        break;

    case e_op_wait:
        VERIFY(!instr_data.cond_action_happened);
        VERIFY(instr_data.rep_count == 0);
        break;

    default:
        VERIFY(!instr_data.cond_action_happened);
        VERIFY(instr_data.rep_count == 0);
        VERIFY(instr_data.wait_n == 0);
    }

    VERIFY(instr_data.wide_odd_transfer_cnt <= instr_data.wide_transfer_cnt);

    return true;
}

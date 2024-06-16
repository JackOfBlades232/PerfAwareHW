#include "simulator.hpp"
#include "print.hpp"
#include "util.hpp"
#include "instruction.hpp"
#include <cassert>

// @TODO: think again if asserts are appropriate here.
//        Where are we dealing with caller mistakes, and where with bad input?

union reg_memory_t {
    u16 w;
    u8 b[2];
};

enum proc_flag_t {
    e_pflag_c = 1 << 0,
    e_pflag_p = 1 << 2,
    e_pflag_a = 1 << 4,
    e_pflag_z = 1 << 6,
    e_pflag_s = 1 << 7,

    e_pflag_o = 1 << 11,
};

enum arifm_op_t {
    e_arifm_add,
    e_arifm_sub
};

// @TODO: I still think the main ip should be in machine!
//        Maybe invert and make the machine visible everywhere?
struct machine_t {
    reg_memory_t registers[e_reg_max];
};

struct tracing_state_t {
    u32 flags = 0;
    u32 registers_used = to_flag(e_reg_ip);
};

static machine_t g_machine = {};
static tracing_state_t g_tracing = {};

void set_simulation_trace_level(u32 flags)
{
    g_tracing.flags = flags;
}

static u16 *reg_w(reg_t reg)
{
    return &g_machine.registers[reg].w;
}

static u32 do_arifm_op(u32 a, u32 b, arifm_op_t op)
{
    switch (op) {
        case e_arifm_add:
            return a+b;
        case e_arifm_sub:
            return a-b;
    }

    return 0;
}

static void set_flag(u16 flag, bool val)
{
    if (val)
        *reg_w(e_reg_flags) |= flag;
    else
        *reg_w(e_reg_flags) &= ~flag;
}

static bool get_flag(u16 flag)
{
    return *reg_w(e_reg_flags) & flag;
}

static void output_flags_content()
{
    proc_flag_t flags[] = { e_pflag_c, e_pflag_p, e_pflag_a, e_pflag_z, e_pflag_s, e_pflag_o };
    const char *flags_names[ARR_CNT(flags)] = { "C", "P", "A", "Z", "S", "O" };
    for (int f = 0; f < ARR_CNT(flags); ++f) {
        if (*reg_w(e_reg_flags) & flags[f])
            output::print("%s", flags_names[f]);
    }
}

static u16 read_reg(reg_access_t access)
{
    assert((access.offset == 0 && access.size == 2) ||
           (access.reg <= e_reg_d && access.size == 1 && access.offset <= 1));

    reg_memory_t *reg_mem = &g_machine.registers[access.reg];
    if (access.size == 2) // => offset = 0
        return reg_mem->w;
    else
        return reg_mem->b[access.offset];
}

static void write_reg(reg_access_t access, u16 val)
{
    assert((access.offset == 0 && access.size == 2) ||
           (access.reg <= e_reg_d && access.size == 1 && access.offset <= 1));
    assert(access.size == 2 || val < (1 << 8));

    g_tracing.registers_used |= to_flag(access.reg);

    reg_memory_t *reg_mem = &g_machine.registers[access.reg];
    u16 prev_reg_content = reg_mem->w;

    if (access.size == 2) // => offset = 0
        reg_mem->w = val;
    else
        reg_mem->b[access.offset] = (u8)val;

    // @TODO: more elegant/general side-effect tracing. No ideas for design now
    if (g_tracing.flags & e_trace_data_mutation) {
        output::print(" ");
        output::print_word_reg(access.reg);
        output::print(":0x%hx->0x%hx", prev_reg_content, reg_mem->w);
    }
}

// @TODO: spec setting for reading EA in mem (in lea)
static u16 read_operand(operand_t op)
{
    switch (op.type) {
        case e_operand_reg:
            return read_reg(op.data.reg);

        case e_operand_imm:
            return op.data.imm;

        case e_operand_mem:
        case e_operand_cs_ip:
            LOGERR("Operand read not implemented for these types");
        case e_operand_none:
            assert(0);
    }

    return 0;
}

static void write_operand(operand_t op, u16 val)
{
    switch (op.type) {
        case e_operand_reg:
            write_reg(op.data.reg, val);
            break;

        case e_operand_imm:
            LOGERR("Can't write to immediate operand");
            assert(0);

        case e_operand_mem:
        case e_operand_cs_ip:
            LOGERR("Operand read not implemented for these types");
        case e_operand_none:
            assert(0);
    }
}

static void update_flags(arifm_op_t op, u32 a, u32 b, u32 res, bool is_wide)
{
    if (g_tracing.flags & e_trace_data_mutation) {
        output::print(" flags:");
        output_flags_content();
    }

    auto is_neg = [](u32 n, bool is_wide) {
        return n & (1 << (is_wide ? 15 : 7));
    };

    set_flag(e_pflag_z, (res & (is_wide ? 0xFFFF : 0xFF)) == 0);
    set_flag(e_pflag_s, is_neg(res, is_wide));

    // @TODO: manual sais parity == even bits, Casey sais even bits in low 8. Which is it?
    //g_machine.flags[e_pflag_p] = count_ones(res, is_wide ? 16 : 8) % 2 == 0;
    set_flag(e_pflag_p, count_ones(res, 8) % 2 == 0);

    // @TODO: I am getting conflicted evidence about c/o/a flags. Investigate, 
    //        May be better to run actual asm on linux. (or find a way on win64?)
    set_flag(e_pflag_c, res >= (1 << (is_wide ? 16 : 8)));

    bool as = is_neg(a, is_wide), bs = is_neg(b, is_wide);
    set_flag(e_pflag_o, 
        as != (bool)(g_machine.registers[e_reg_flags].w & e_pflag_s) && 
        (
            (op == e_arifm_add && as == bs) ||
            (op == e_arifm_sub && as != bs)
        ));

    set_flag(e_pflag_a, do_arifm_op(a & 0xF, b & 0xF, op) & 0x10);

    if (g_tracing.flags & e_trace_data_mutation) {
        output::print("->");
        output_flags_content();
    }
}

void cond_jump(u16 disp, bool cond)
{
    if (cond)
        *reg_w(e_reg_ip) += disp;
}

void cx_loop_jump(u16 disp, i16 delta_cx, bool cond = true)
{
    reg_access_t cx = get_word_reg_access(e_reg_c);
    if (delta_cx)
        write_reg(cx, read_reg(cx) + delta_cx);

    if (read_reg(cx) != 0 && cond)
        *reg_w(e_reg_ip) += disp;
}

void simulate_instruction_execution(instruction_t instr, u32 *ip)
{
    if (g_tracing.flags & e_trace_disassembly)
        output::print_intstruction(instr);
    if (g_tracing.flags & e_trace_data_mutation)
        output::print(" ;");

    // @IDEA: do we even need readback on every instr? Maybe just return ip?
    *reg_w(e_reg_ip) = *ip;

    u32 prev_ip = *reg_w(e_reg_ip);
    *reg_w(e_reg_ip) += instr.size;

    // @TODO: correct instruction format validation
    switch (instr.op) {
        case e_op_mov:
            write_operand(instr.operands[0], read_operand(instr.operands[1]));
            break;

        case e_op_add: {
            u32 dst = read_operand(instr.operands[0]);
            u32 src = read_operand(instr.operands[1]);
            u32 res = dst + src;
            write_operand(instr.operands[0], res);
            update_flags(e_arifm_add, dst, src, res, instr.flags & e_iflags_w);
        } break;

        case e_op_sub: {
            u32 dst = read_operand(instr.operands[0]);
            u32 src = read_operand(instr.operands[1]);
            u32 res = dst - src;
            write_operand(instr.operands[0], res);
            update_flags(e_arifm_sub, dst, src, res, instr.flags & e_iflags_w);
        } break;

        case e_op_cmp: {
            u32 dst = read_operand(instr.operands[0]);
            u32 src = read_operand(instr.operands[1]);
            update_flags(e_arifm_sub, dst, src, dst - src, instr.flags & e_iflags_w);
        } break;

        case e_op_je:
            cond_jump(read_operand(instr.operands[0]), get_flag(e_pflag_z));
            break;
        case e_op_jl:
            cond_jump(read_operand(instr.operands[0]), get_flag(e_pflag_s) && !get_flag(e_pflag_z));
            break;
        case e_op_jle:
            cond_jump(read_operand(instr.operands[0]), get_flag(e_pflag_s) || get_flag(e_pflag_z));
            break;
        case e_op_jb:
            cond_jump(read_operand(instr.operands[0]), get_flag(e_pflag_c) && !get_flag(e_pflag_z));
            break;
        case e_op_jbe:
            cond_jump(read_operand(instr.operands[0]), get_flag(e_pflag_c) || get_flag(e_pflag_z));
            break;
        case e_op_jp:
            cond_jump(read_operand(instr.operands[0]), get_flag(e_pflag_p));
            break;
        case e_op_jo:
            cond_jump(read_operand(instr.operands[0]), get_flag(e_pflag_o));
            break;
        case e_op_js:
            cond_jump(read_operand(instr.operands[0]), get_flag(e_pflag_s));
            break;
        case e_op_jne:
            cond_jump(read_operand(instr.operands[0]), !get_flag(e_pflag_z));
            break;
        case e_op_jge:
            cond_jump(read_operand(instr.operands[0]), !get_flag(e_pflag_s) || get_flag(e_pflag_z));
            break;
        case e_op_jg:
            cond_jump(read_operand(instr.operands[0]), !get_flag(e_pflag_s) && !get_flag(e_pflag_z));
            break;
        case e_op_jae:
            cond_jump(read_operand(instr.operands[0]), !get_flag(e_pflag_c) || get_flag(e_pflag_z));
            break;
        case e_op_ja:
            cond_jump(read_operand(instr.operands[0]), !get_flag(e_pflag_c) && !get_flag(e_pflag_z));
            break;
        case e_op_jnp:
            cond_jump(read_operand(instr.operands[0]), !get_flag(e_pflag_p));
            break;
        case e_op_jno:
            cond_jump(read_operand(instr.operands[0]), !get_flag(e_pflag_o));
            break;
        case e_op_jns:
            cond_jump(read_operand(instr.operands[0]), !get_flag(e_pflag_s));
            break;

        case e_op_loop:
            cx_loop_jump(read_operand(instr.operands[0]), -1);
            break;
        case e_op_loopz:
            cx_loop_jump(read_operand(instr.operands[0]), -1, get_flag(e_pflag_z));
            break;
        case e_op_loopnz:
            cx_loop_jump(read_operand(instr.operands[0]), -1, !get_flag(e_pflag_z));
            break;
        case e_op_jcxz:
            cx_loop_jump(read_operand(instr.operands[0]), 0);
            break;

        default:
            LOGERR("Instruction execution not implemented");
            assert(0);
    }

    if (g_tracing.flags & e_trace_data_mutation)
        output::print(" ip:0x%hx->0x%hx", prev_ip, g_machine.registers[e_reg_ip].w);
    if (g_tracing.flags & (e_trace_disassembly | e_trace_data_mutation))
        output::print("\n");

    *ip = *reg_w(e_reg_ip);
}

void output_simulation_results()
{
    if (g_tracing.flags != 0)
        output::print("\n");
    output::print("Registers state:\n");
    for (int reg = e_reg_a; reg < e_reg_flags; ++reg) {
        if (g_tracing.registers_used & to_flag(reg)) {
            u16 val = read_reg(get_word_reg_access((reg_t)reg));
            output::print("      ");
            output::print_word_reg((reg_t)reg);
            output::print(": 0x%04hx (%hu)\n", val, val);
        }
    }
    output::print("   flags: ");
    output_flags_content();
    output::print("\n");
}

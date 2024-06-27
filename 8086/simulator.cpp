#include "simulator.hpp"
#include "memory.hpp"
#include "print.hpp"
#include "util.hpp"
#include "clocks.hpp"
#include "instruction.hpp"
#include <cassert>

// @TODO: think again if asserts are appropriate here.
//        Where are we dealing with caller mistakes, and where with bad input?

// @TODO: warning output when tracing cycles

static constexpr u32 c_seg_size = POT(16);

enum proc_flag_t {
    e_pflag_c = 1 << 0,
    e_pflag_p = 1 << 2,
    e_pflag_a = 1 << 4,
    e_pflag_z = 1 << 6,
    e_pflag_s = 1 << 7,

    e_pflag_o = 1 << 11,
};

// @TODO: what if I use 2-s complemet, will it remove the need for classification?
enum arifm_op_t {
    e_arifm_add,
    e_arifm_sub
};

struct machine_t {
    u16 registers[e_reg_max];
};

struct tracing_state_t {
    u32 flags = 0;
    u32 registers_used = to_flag(e_reg_ip);
    u32 total_cycles = 0;
};

static machine_t g_machine = {};
static tracing_state_t g_tracing = {};

void set_simulation_trace_level(u32 flags)
{
    g_tracing.flags = flags;
}

// @TODO: i don't like this abstraction
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

static bool is_zero(u32 n, bool is_wide)
{
    return (n & (is_wide ? 0xFFFF : 0xFF)) == 0;
};

static bool is_neg(u32 n, bool is_wide)
{
    return n & (1 << (is_wide ? 15 : 7));
};

static bool is_parity(u32 n)
{
    return count_ones(n, 8) % 2 == 0;
}

static void set_flag(u16 flag, bool val)
{
    if (val)
        g_machine.registers[e_reg_flags] |= flag;
    else
        g_machine.registers[e_reg_flags] &= ~flag;
}

static bool get_flag(u16 flag)
{
    return g_machine.registers[e_reg_flags] & flag;
}

static void output_flags_content(u16 flags_val)
{
    proc_flag_t flags[] = { e_pflag_c, e_pflag_p, e_pflag_a, e_pflag_z, e_pflag_s, e_pflag_o };
    const char *flags_names[ARR_CNT(flags)] = { "C", "P", "A", "Z", "S", "O" };
    for (int f = 0; f < ARR_CNT(flags); ++f) {
        if (flags_val & flags[f])
            output::print("%s", flags_names[f]);
    }
}

static u16 read_reg(reg_access_t access)
{
    assert((access.offset == 0 && access.size == 2) ||
           (access.reg <= e_reg_d && access.size == 1 && access.offset <= 1));

    if (access.size == 2) // => offset = 0
        return g_machine.registers[access.reg];
    else
        return g_machine.registers[access.reg] >> (access.offset * 8);
}

static void write_reg(reg_access_t access, u16 val)
{
    assert((access.offset == 0 && access.size == 2) ||
           (access.reg <= e_reg_d && access.size == 1 && access.offset <= 1));

    g_tracing.registers_used |= to_flag(access.reg);

    u16 *reg_mem = &g_machine.registers[access.reg];
    u16 prev_reg_content = *reg_mem;

    if (access.size == 2) // => offset = 0
        *reg_mem = val;
    else
        set_byte(reg_mem, (u8)val, access.offset);

    // @TODO: more elegant/general side-effect tracing. No ideas for design now
    if (g_tracing.flags & e_trace_data_mutation) {
        output::print(" ");
        output::print_word_reg(access.reg);
        output::print(":0x%hx->0x%hx", prev_reg_content, *reg_mem);
    }
}

static u16 calculate_ea(ea_mem_access_t access)
{
    constexpr reg_t c_null_reg = e_reg_max;
    const reg_t ops[e_ea_base_max][2] =
    {
        {e_reg_b,    e_reg_si},
        {e_reg_b,    e_reg_di},
        {e_reg_bp,   e_reg_si},
        {e_reg_bp,   e_reg_di},
        {e_reg_si,   c_null_reg},
        {e_reg_di,   c_null_reg},
        {e_reg_bp,   c_null_reg},
        {e_reg_b,    c_null_reg},
        {c_null_reg, c_null_reg},
    };

    reg_t r1 = ops[access.base][0];
    reg_t r2 = ops[access.base][1];
    u16 op1 = r1 == c_null_reg ? 0 : read_reg(get_word_reg_access(r1));
    u16 op2 = r2 == c_null_reg ? 0 : read_reg(get_word_reg_access(r2));
    return op1 + op2 + access.disp;
}

// @TODO: double check manual for segment tricks
static memory_access_t get_segment_access(ea_mem_access_t access, const instruction_t *instr)
{
    memory_access_t seg = get_main_memory_access();
    seg.size = c_seg_size;

    if (instr->flags & e_iflags_seg_override)
        seg.base = g_machine.registers[instr->segment_override];

    // @TODO: check in the manual
    if (access.base == e_ea_base_bp)
        seg.base = g_machine.registers[e_reg_ss];
    else
        seg.base = g_machine.registers[e_reg_ds];

    seg.base <<= 4;

    return seg;
}

static u16 read_mem(ea_mem_access_t access, const instruction_t *instr)
{
    memory_access_t seg_mem = get_segment_access(access, instr);
    u16 offset = calculate_ea(access);

    if (instr->flags & e_iflags_w)
        return read_byte_at(seg_mem, offset) | (read_byte_at(seg_mem, offset+1) << 8);
    else
        return read_byte_at(seg_mem, offset);
}

static void write_mem(ea_mem_access_t access, u16 val, const instruction_t *instr)
{
    memory_access_t seg_mem = get_segment_access(access, instr);
    u16 offset = calculate_ea(access);

    u16 prev_content = read_mem(access, instr);

    if (instr->flags & e_iflags_w) {
        // The simulated 8086 is little endian
        write_byte_to(seg_mem, offset, val & 0xFF);
        write_byte_to(seg_mem, offset+1, val >> 8);
    } else
        write_byte_to(seg_mem, offset, val);

    if (g_tracing.flags & e_trace_data_mutation) {
        u32 addr = get_full_address(seg_mem, offset);
        output::print(" ");
        if (instr->flags & e_iflags_w)
            output::print("[0x%x-0x%x]", addr, addr+1);
        else
            output::print("[0x%x]", addr);
        output::print(":0x%hx->0x%hx", prev_content, read_mem(access, instr));
    }
}

// @TODO: spec setting for reading EA in mem (in lea)
static u16 read_operand(operand_t op, const instruction_t *instr = nullptr)
{
    switch (op.type) {
        case e_operand_reg:
            return read_reg(op.data.reg);

        case e_operand_imm:
            return op.data.imm;

        case e_operand_mem:
            return read_mem(op.data.mem, instr);

        case e_operand_cs_ip:
            LOGERR("Operand read not implemented for these types");
        case e_operand_none:
            assert(0);
    }

    return 0;
}

static void write_operand(operand_t op, u16 val, const instruction_t *instr = nullptr)
{
    switch (op.type) {
        case e_operand_reg:
            write_reg(op.data.reg, val);
            break;

        case e_operand_mem:
            write_mem(op.data.mem, val, instr);
            break;

        case e_operand_imm:
            LOGERR("Can't write to immediate operand");
            assert(0);

        case e_operand_cs_ip:
            LOGERR("Operand read not implemented for these types");
        case e_operand_none:
            assert(0);
    }
}

// @TODO: better masking?
static void update_flags_arifm(arifm_op_t op, u32 a, u32 b, u32 res,
                               bool is_wide, u16 ignore_mask = 0x0)
{
    u16 mask = ~ignore_mask;

    set_flag(e_pflag_z & mask, is_zero(res, is_wide));
    set_flag(e_pflag_s & mask, is_neg(res, is_wide));
    set_flag(e_pflag_p & mask, is_parity(res));
    set_flag(e_pflag_c & mask, res >= (1 << (is_wide ? 16 : 8)));

    bool as = is_neg(a, is_wide), bs = is_neg(b, is_wide);
    set_flag(e_pflag_o & mask, 
        as != (bool)(g_machine.registers[e_reg_flags] & e_pflag_s) && 
        (
            (op == e_arifm_add && as == bs) ||
            (op == e_arifm_sub && as != bs)
        ));

    set_flag(e_pflag_a & mask, do_arifm_op(a & 0xF, b & 0xF, op) & 0x10);
}

// @TODO: check abstraction
// @TODO: pull out flag setting with masks and action types, maybe
static void update_flags_logic(u32 res, bool is_wide)
{
    set_flag(e_pflag_o, 0);
    set_flag(e_pflag_c, 0);
    set_flag(e_pflag_z, is_zero(res, is_wide));
    set_flag(e_pflag_s, is_neg(res, is_wide));
    set_flag(e_pflag_p, is_parity(res));
}

bool cond_jump(u16 disp, bool cond)
{
    if (cond)
        g_machine.registers[e_reg_ip] += disp;

    return cond;
}

bool cx_loop_jump(u16 disp, i16 delta_cx, bool cond = true)
{
    reg_access_t cx = get_word_reg_access(e_reg_c);
    if (delta_cx)
        write_reg(cx, read_reg(cx) + delta_cx);

    if (read_reg(cx) != 0 && cond) {
        g_machine.registers[e_reg_ip] += disp;
        return true;
    }

    return false;
}

u32 simulate_instruction_execution(instruction_t instr)
{
    if (g_tracing.flags & e_trace_disassembly)
        output::print_intstruction(instr);
    if (g_tracing.flags & (e_trace_data_mutation | e_trace_cycles))
        output::print(" ;");

    u16 prev_flags = g_machine.registers[e_reg_flags];
    u32 prev_ip = g_machine.registers[e_reg_ip];
    
    g_machine.registers[e_reg_ip] += instr.size;

    bool w = instr.flags & e_iflags_w;

    bool cond_action_happened = false;

    // @TODO: correct instruction format validation

    // @TODO: set global instruction each iter to a context so as not to pass it around?
    switch (instr.op) {
        case e_op_mov: {
            u32 src = read_operand(instr.operands[1], &instr);
            write_operand(instr.operands[0], src, &instr);
        } break;

        case e_op_add: {
            u32 dst = read_operand(instr.operands[0], &instr);
            u32 src = read_operand(instr.operands[1], &instr);
            u32 res = dst + src;
            write_operand(instr.operands[0], res, &instr);
            update_flags_arifm(e_arifm_add, dst, src, res, w);
        } break;

        case e_op_sub: {
            u32 dst = read_operand(instr.operands[0], &instr);
            u32 src = read_operand(instr.operands[1], &instr);
            u32 res = dst - src;
            write_operand(instr.operands[0], res, &instr);
            update_flags_arifm(e_arifm_sub, dst, src, res, w);
        } break;

        case e_op_cmp: {
            u32 dst = read_operand(instr.operands[0], &instr);
            u32 src = read_operand(instr.operands[1], &instr);
            update_flags_arifm(e_arifm_sub, dst, src, dst - src, w);
        } break;

        case e_op_xor: {
            u32 dst = read_operand(instr.operands[0], &instr);
            u32 src = read_operand(instr.operands[1], &instr);
            u32 res = dst ^ src;
            write_operand(instr.operands[0], res, &instr);
            update_flags_logic(res, w);
        } break;

        case e_op_test: {
            u32 dst = read_operand(instr.operands[0], &instr);
            u32 src = read_operand(instr.operands[1], &instr);
            u32 res = dst & src;
            update_flags_logic(res, w);
        } break;

        case e_op_inc: {
            u32 dst = read_operand(instr.operands[0], &instr);
            u32 res = dst + 1;
            write_operand(instr.operands[0], res, &instr);
            update_flags_arifm(e_arifm_add, dst, 1, res, w);
        } break;

        case e_op_dec: {
            u32 dst = read_operand(instr.operands[0], &instr);
            u32 res = dst - 1;
            write_operand(instr.operands[0], res, &instr);
            update_flags_arifm(e_arifm_sub, dst, 1, res, w);
        } break;

        case e_op_je:
            cond_action_happened = cond_jump(read_operand(instr.operands[0]), get_flag(e_pflag_z));
            break;
        case e_op_jl:
            cond_action_happened = cond_jump(read_operand(instr.operands[0]), get_flag(e_pflag_s) && !get_flag(e_pflag_z));
            break;
        case e_op_jle:
            cond_action_happened = cond_jump(read_operand(instr.operands[0]), get_flag(e_pflag_s) || get_flag(e_pflag_z));
            break;
        case e_op_jb:
            cond_action_happened = cond_jump(read_operand(instr.operands[0]), get_flag(e_pflag_c) && !get_flag(e_pflag_z));
            break;
        case e_op_jbe:
            cond_action_happened = cond_jump(read_operand(instr.operands[0]), get_flag(e_pflag_c) || get_flag(e_pflag_z));
            break;
        case e_op_jp:
            cond_action_happened = cond_jump(read_operand(instr.operands[0]), get_flag(e_pflag_p));
            break;
        case e_op_jo:
            cond_action_happened = cond_jump(read_operand(instr.operands[0]), get_flag(e_pflag_o));
            break;
        case e_op_js:
            cond_action_happened = cond_jump(read_operand(instr.operands[0]), get_flag(e_pflag_s));
            break;
        case e_op_jne:
            cond_action_happened = cond_jump(read_operand(instr.operands[0]), !get_flag(e_pflag_z));
            break;
        case e_op_jge:
            cond_action_happened = cond_jump(read_operand(instr.operands[0]), !get_flag(e_pflag_s) || get_flag(e_pflag_z));
            break;
        case e_op_jg:
            cond_action_happened = cond_jump(read_operand(instr.operands[0]), !get_flag(e_pflag_s) && !get_flag(e_pflag_z));
            break;
        case e_op_jae:
            cond_action_happened = cond_jump(read_operand(instr.operands[0]), !get_flag(e_pflag_c) || get_flag(e_pflag_z));
            break;
        case e_op_ja:
            cond_action_happened = cond_jump(read_operand(instr.operands[0]), !get_flag(e_pflag_c) && !get_flag(e_pflag_z));
            break;
        case e_op_jnp:
            cond_action_happened = cond_jump(read_operand(instr.operands[0]), !get_flag(e_pflag_p));
            break;
        case e_op_jno:
            cond_action_happened = cond_jump(read_operand(instr.operands[0]), !get_flag(e_pflag_o));
            break;
        case e_op_jns:
            cond_action_happened = cond_jump(read_operand(instr.operands[0]), !get_flag(e_pflag_s));
            break;

        case e_op_loop:
            cond_action_happened = cx_loop_jump(read_operand(instr.operands[0]), -1);
            break;
        case e_op_loopz:
            cond_action_happened = cx_loop_jump(read_operand(instr.operands[0]), -1, get_flag(e_pflag_z));
            break;
        case e_op_loopnz:
            cond_action_happened = cx_loop_jump(read_operand(instr.operands[0]), -1, !get_flag(e_pflag_z));
            break;
        case e_op_jcxz:
            cond_action_happened = cx_loop_jump(read_operand(instr.operands[0]), 0);
            break;

        default:
            LOGERR("Instruction execution not implemented");
            assert(0);
    }

    if (g_tracing.flags & e_trace_data_mutation) {
        output::print(" ip:0x%hx->0x%hx", prev_ip, g_machine.registers[e_reg_ip]);

        if (prev_flags != g_machine.registers[e_reg_flags]) {
            output::print(" flags:");
            output_flags_content(prev_flags);
            output::print("->");
            output_flags_content(g_machine.registers[e_reg_flags]);
        }
    }
    if (g_tracing.flags & e_trace_cycles) {
        u32 elapsed_cycles = estimate_instruction_clocks(instr, cond_action_happened);
        g_tracing.total_cycles += elapsed_cycles;
        output::print(" | Clocks: +%u=%u", elapsed_cycles, g_tracing.total_cycles);
    }

    if (g_tracing.flags)
        output::print("\n");

    // @TODO: pull this out with mem accesses?
    return (g_machine.registers[e_reg_cs] << 4) | g_machine.registers[e_reg_ip];
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
    output_flags_content(g_machine.registers[e_reg_flags]);
    output::print("\n");
}

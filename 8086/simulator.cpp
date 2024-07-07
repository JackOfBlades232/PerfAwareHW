#include "simulator.hpp"
#include "consts.hpp"
#include "memory.hpp"
#include "print.hpp"
#include "util.hpp"
#include "clocks.hpp"
#include "instruction.hpp"
#include <cassert>
#include <cstdlib>

// @TODO: think again if asserts are appropriate here.
//        Where are we dealing with caller mistakes, and where with bad input?

// @TODO: warning output when tracing cycles

// @TODO: in-out ports side effect tracing?

// @TODO: get rid of convoluted access calls, further

// @TODO: nop instr?

static constexpr u32 c_seg_size = POT(16);

enum proc_flag_t {
    e_pflag_c = 1 << 0,
    e_pflag_p = 1 << 2,
    e_pflag_a = 1 << 4,
    e_pflag_z = 1 << 6,
    e_pflag_s = 1 << 7,

    e_pflag_o = 1 << 11,
};

// @TODO: global endian checks and macros for h/l regs

struct machine_t {
    union {
        struct {
            u16 a;
            u16 b;
            u16 c;
            u16 d;
            u16 sp;
            u16 bp;
            u16 si;
            u16 di;
            u16 es;
            u16 cs;
            u16 ss;
            u16 ds;
            u16 ip;
            u16 flags;
        };
        u16 regs[e_reg_max];
    };
};

struct tracing_state_t {
    u32 flags = 0;
    u32 registers_used = to_flag(e_reg_ip);
    u32 total_cycles = 0;
};

static machine_t g_machine = {};
static tracing_state_t g_tracing = {};

void set_simulation_trace_option(u32 flags, bool set)
{
    set_flags(&g_tracing.flags, flags, set);
}

static u32 hmask(bool is_wide)
{
    return 1 << (is_wide ? 15 : 7);
}

static bool is_zero(u32 n, bool is_wide)
{
    return (n & (is_wide ? 0xFFFF : 0xFF)) == 0;
};

static bool is_neg(u32 n, bool is_wide)
{
    return n & hmask(is_wide);
};

static bool is_parity(u32 n)
{
    return count_ones(n, 8) % 2 == 0;
}

static bool is_arifm_carry(u32 n, bool is_wide)
{
    return n & (hmask(is_wide) << 1);
}

static void set_pflag(u16 flag, bool val)
{
    if (val)
        g_machine.flags |= flag;
    else
        g_machine.flags &= ~flag;
}

static bool get_pflag(u16 flag)
{
    return g_machine.flags & flag;
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

static u16 read_val_at(memory_access_t at, u32 offset, bool is_wide)
{
    if (is_wide)
        return read_word_at(at, offset);
    else
        return read_byte_at(at, offset);
}

static void write_val_to(memory_access_t to, u32 offset, u32 val, bool is_wide)
{
    if (is_wide)
        write_word_to(to, offset, val);
    else
        write_byte_to(to, offset, val);
}

static u16 read_reg(reg_access_t access)
{
    assert((access.offset == 0 && access.size == 2) ||
           (access.reg <= e_reg_d && access.size == 1 && access.offset <= 1));

    if (access.size == 2) // => offset = 0
        return g_machine.regs[access.reg];
    else
        return (g_machine.regs[access.reg] >> (access.offset * 8)) & 0xFF;
}

static void write_reg(reg_access_t access, u16 val)
{
    assert((access.offset == 0 && access.size == 2) ||
           (access.reg <= e_reg_d && access.size == 1 && access.offset <= 1));

    g_tracing.registers_used |= to_flag(access.reg);

    u16 *reg_mem = &g_machine.regs[access.reg];
    if (access.size == 2) // => offset = 0
        *reg_mem = val;
    else
        set_byte(reg_mem, (u8)val, access.offset);
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

static u32 get_address_in_segment(u16 seg_val, u16 offset)
{
    return ((u32)seg_val << 4) + offset;
}

static memory_access_t get_segment_access(reg_t seg_reg)
{
    memory_access_t seg = get_main_memory_access();
    seg.size = c_seg_size;
    seg.base += get_address_in_segment(g_machine.regs[seg_reg], 0);
    return seg;
}

// @TODO: double check manual for segment tricks
static memory_access_t get_segment_access_for_ea(ea_mem_access_t access, reg_t seg_override)
{
    if (seg_override != e_reg_max) {
        assert(seg_override == e_reg_cs ||
               seg_override == e_reg_ss ||
               seg_override == e_reg_ds ||
               seg_override == e_reg_es);
        return get_segment_access(seg_override);
    }

    // @TODO: check in the manual, es for string instructions?
    if (access.base == e_ea_base_bp)
        return get_segment_access(e_reg_ss);
    else
        return get_segment_access(e_reg_ds);
}

static u16 read_mem(ea_mem_access_t access, bool is_wide, reg_t seg_override)
{
    memory_access_t seg_mem = get_segment_access_for_ea(access, seg_override);
    u16 offset = calculate_ea(access);

    return read_val_at(seg_mem, offset, is_wide);
}

static void write_mem(ea_mem_access_t access, u16 val, bool is_wide, reg_t seg_override)
{
    memory_access_t seg_mem = get_segment_access_for_ea(access, seg_override);
    u16 offset = calculate_ea(access);

    u16 prev_content = read_mem(access, is_wide, seg_override);

    write_val_to(seg_mem, offset, val, is_wide);

    if (g_tracing.flags & e_trace_data_mutation) {
        u32 addr = get_full_address(seg_mem, offset);
        output::print(" ");
        if (is_wide)
            output::print("[0x%x-0x%x]", addr, addr+1);
        else
            output::print("[0x%x]", addr);
        output::print(":0x%hx->0x%hx", prev_content, read_mem(access, is_wide, seg_override));
    }
}

static u16 read_operand(operand_t op, bool is_wide, reg_t seg_override = e_reg_max)
{
    switch (op.type) {
        case e_operand_reg:
            return read_reg(op.data.reg);

        case e_operand_imm:
            return op.data.imm;

        case e_operand_mem:
            return read_mem(op.data.mem, is_wide, seg_override);

        case e_operand_cs_ip:
            LOGERR("Operand read not implemented for these types");
        case e_operand_none:
            assert(0);
    }

    return 0;
}

static void write_operand(operand_t op, u16 val, bool is_wide, reg_t seg_override = e_reg_max)
{
    switch (op.type) {
        case e_operand_reg:
            write_reg(op.data.reg, val);
            break;

        case e_operand_mem:
            write_mem(op.data.mem, val, is_wide, seg_override);
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

static u16 read_stack(bool is_wide)
{
    memory_access_t stack = get_segment_access(e_reg_ss);
    return read_val_at(stack, g_machine.sp, is_wide);
}

static void write_stack(u16 val, bool is_wide)
{
    memory_access_t stack = get_segment_access(e_reg_ss);
    write_val_to(stack, g_machine.sp, val, is_wide);
}

static void update_common_flags(u32 res, bool is_wide)
{
    set_pflag(e_pflag_z, is_zero(res, is_wide));
    set_pflag(e_pflag_s, is_neg(res, is_wide));
    set_pflag(e_pflag_p, is_parity(res));
}

static void update_arifm_flags(u32 a, i32 b, bool is_wide)
{
    u32 res = (u32)((i32)a + b);
    update_common_flags(res, is_wide);
    set_pflag(e_pflag_c, is_arifm_carry(res, is_wide));
    set_pflag(e_pflag_o, is_neg(a, is_wide) == is_neg(b, is_wide) &&
                         is_neg(a, is_wide) != is_neg(res, is_wide));
    set_pflag(e_pflag_a, ((0xF & a) + sgn(b)*(0xF & abs(b))) & 0x10);
}

static void update_logic_flags(u32 res, bool is_wide)
{
    update_common_flags(res, is_wide);
    set_pflag(e_pflag_c, 0);
    set_pflag(e_pflag_o, 0);
    set_pflag(e_pflag_a, 0);
}

static void update_shift_flags(bool pushed_bit, u32 res, u32 orig, bool is_wide)
{
    set_pflag(e_pflag_c, pushed_bit);
    set_pflag(e_pflag_o, is_neg(res, is_wide) != is_neg(orig, is_wide));
}

static bool cond_jump(u16 disp, bool cond)
{
    if (cond)
        g_machine.ip += disp;

    return cond;
}

static bool cx_loop_jump(u16 disp, i16 delta_cx, bool cond = true)
{
    reg_access_t cx = get_word_reg_access(e_reg_c);
    if (delta_cx)
        write_reg(cx, read_reg(cx) + delta_cx);

    if (read_reg(cx) != 0 && cond) {
        g_machine.ip += disp;
        return true;
    }

    return false;
}

static u32 get_full_ip()
{
    return get_address_in_segment(g_machine.cs, g_machine.ip);
}

u32 get_simulation_ip()
{
    return get_full_ip();
}

u32 simulate_instruction_execution(instruction_t instr)
{
    // @TODO: s flag for add is already needed?
    bool w = instr.flags & e_iflags_w;
    int op_bytes = w ? 2 : 1;
    int op_bits  = op_bytes * 8;
    int op_mask  = n_bit_mask(op_bits);

    reg_t seg_override = (instr.flags & e_iflags_seg_override) ?
                         instr.segment_override : e_reg_max;

    operand_t op0 = instr.operands[0];
    operand_t op1 = instr.operands[1];
    u32 op0_val = op0.type == e_operand_none ? 0 : read_operand(op0, w, seg_override);
    u32 op1_val = op1.type == e_operand_none ? 0 : read_operand(op1, w, seg_override);

    instruction_metadata_t metadata = {};
    metadata.instr = instr;
    metadata.op0_val = op0_val;
    metadata.op1_val = op1_val;

    bool zf = get_pflag(e_pflag_z);
    bool sf = get_pflag(e_pflag_s);
    bool pf = get_pflag(e_pflag_p);
    bool of = get_pflag(e_pflag_o);
    bool cf = get_pflag(e_pflag_c);
    bool af = get_pflag(e_pflag_a);

    if ((instr.op == e_op_ret || instr.op == e_op_retf) &&
        (g_tracing.flags & e_trace_stop_on_ret))
    {
        output::print("STOPONRET: Return encountered at address %u\n", get_full_ip());
        return c_ip_terminate; // exits loop
    }

    machine_t prev_machine = g_machine;
    
    g_machine.ip += instr.size;

    if (g_tracing.flags & e_trace_disassembly)
        output::print_instruction(instr);
    if (g_tracing.flags & (e_trace_data_mutation | e_trace_cycles))
        output::print(" ;");

    // @TODO: correct instruction format validation
    // (as a separate module, it could be useful in decoder/main loop,
    //  since the whole system relies on this invariant)

    auto nop_effect = []() {
        if (g_tracing.flags & e_trace_data_mutation) 
            output::print(" nop");
    };

    switch (instr.op) {
        case e_op_mov:
            write_operand(op0, op1_val, w, seg_override);
            break;

        case e_op_push:
            write_stack(op0_val, w);
            g_machine.sp -= op_bytes;
            break;

        case e_op_pop:
           // @TODO: restrict popping cs
            write_operand(op0, read_stack(w), w);
            g_machine.sp += op_bytes;
            break;

        case e_op_xchg:
            write_operand(op0, op1_val, w, seg_override);
            write_operand(op1, op0_val, w, seg_override);
            break;

        case e_op_xlat: {
            reg_access_t al = get_low_byte_reg_access(e_reg_a);
            ea_mem_access_t access = {e_ea_base_bx, (u8)read_reg(al)};
            write_reg(al, read_mem(access, false, e_reg_max));
        } break;

        case e_op_lea:
            write_operand(op0, calculate_ea(op1.data.mem), true);
            break;

        case e_op_lds:
        case e_op_les: {
            ea_mem_access_t base_mem = op1.data.mem;
            base_mem.disp += 2; // high 16bits
            u16 base = read_mem(base_mem, w, seg_override);
            write_operand(op0, op1_val, w);
            (instr.op == e_op_lds ? g_machine.ds : g_machine.es) = base;
        } break;

        case e_op_lahf:
            write_reg(get_high_byte_reg_access(e_reg_a),
                      g_machine.flags &
                      (e_pflag_s | e_pflag_z | e_pflag_a | e_pflag_p | e_pflag_c));
            break;

        case e_op_sahf: {
            u16 ah = g_machine.a >> 8;
            set_pflag(e_pflag_s, ah & e_pflag_s);
            set_pflag(e_pflag_z, ah & e_pflag_z);
            set_pflag(e_pflag_a, ah & e_pflag_a);
            set_pflag(e_pflag_p, ah & e_pflag_p);
            set_pflag(e_pflag_c, ah & e_pflag_c);
        } break;

        // @TODO: pull out stach operations
        case e_op_pushf:
            write_stack(g_machine.flags, true);
            g_machine.sp -= 2;
            break;

        case e_op_popf:
            g_machine.flags = read_stack(true);
            g_machine.sp += 2;
            break;

        case e_op_adc:
            if (get_pflag(e_pflag_c))
                ++op1_val;
        case e_op_add:
            update_arifm_flags(op0_val, op1_val, w);
            write_operand(op0, op0_val + op1_val, w, seg_override);
            break;

        // @TODO: rewrite more clearly, check against microcode and manual
        case e_op_aaa:
        case e_op_aas:
        case e_op_daa:
        case e_op_das: {
            reg_access_t al = get_low_byte_reg_access(e_reg_a);
            u8 res = read_reg(al);
            bool is_add = instr.op == e_op_aaa || instr.op == e_op_daa;
            bool carry;
            if (instr.op == e_op_aaa || instr.op == e_op_aas) {
                carry = res >= 10;
                g_machine.a += (carry ? 1 << 8 : 0) * (is_add ? 1 : -1); // ++AH or --AH
                write_reg(al, carry ? res - 10 : res);
            } else { // @TODO: is there really no difference in daa/das?
                u8 lo = res & 0xF;
                u8 hi = res >> 4;
                carry = lo >= 10 || hi >= 10;
                if (lo >= 10) {
                    lo -= 10;
                    ++hi;
                }
                if (hi >= 10)
                    hi -= 10;
                res = (hi << 4) | lo;
                update_common_flags(res, false);
                write_reg(al, res);
            }
            set_pflag(e_pflag_c, carry);
            set_pflag(e_pflag_a, carry);
        } break;

        case e_op_sbb:
            if (get_pflag(e_pflag_c))
                --op1_val;
        case e_op_sub:
            update_arifm_flags(op0_val, -op1_val, w);
            write_operand(op0, op0_val - op1_val, w, seg_override);
            break;

        case e_op_cmp:
            update_arifm_flags(op0_val, -op1_val, w);
            break;

        case e_op_mul: {
            u32 res = op0_val * op1_val;
            if (w) {
                g_machine.a = res & 0xFFFF;
                g_machine.d = res >> 16;
            } else
                g_machine.a = res;
            u16 hi = res >> op_bits;
            set_pflag(e_pflag_c, hi != 0);
            set_pflag(e_pflag_o, hi != 0);
        } break;

        case e_op_imul: {
            // @TODO: check conversion, one cast ok?
            u32 res = (i32)op0_val * (i32)op1_val;
            // @TODO: pull out, refac
            if (w) {
                g_machine.a = res & 0xFFFF;
                g_machine.d = res >> 16;
            } else
                g_machine.a = res;
            u16 hi = res >> op_bits;
            u16 lo = res & op_mask;
            bool hi_is_sign_ext = (hi == op_mask && (lo & hmask(w))) ||
                                  (hi == 0 && !(lo & hmask(w)));
            set_pflag(e_pflag_c, hi_is_sign_ext);
            set_pflag(e_pflag_o, hi_is_sign_ext);
        } break;

        // @TODO: move/merge?
        case e_op_aam: {
            u16 res = g_machine.a;
            u8 lo = res & 0xFF;
            u8 hi = res >> 8;
            u8 carries = lo % 10;
            lo -= 10 * carries;
            hi += carries;
            g_machine.a = res;
            // @TODO: sais so on felix cite, not manual, verify
            update_common_flags(lo, false);
        } break;

        // @TODO: sort out max remainders' checking for div and idiv
        case e_op_div:
        case e_op_idiv: {
            if (op0_val == 0) {
                // @TODO: generate int 0 instead, and log it properly
                // And make a test that actually does something useful
                LOGERR("Division by 0 exception");
                //exit(1);
                break;
            }
            u32 whole = w ? ((g_machine.d << 16) | g_machine.a) : g_machine.a;
            u16 quot, rem;
            if (instr.op == e_op_div) {
                quot = whole / op0_val;
                rem  = whole - quot*op0_val;
            } else {
                // @TODO: check conversion, one cast ok?
                quot = (i32)whole / (i32)op0_val;
                rem  = (i32)whole - (i32)quot*(i32)op0_val;
            }
            if (w) {
                g_machine.a = quot;
                g_machine.d = rem;
            } else
                g_machine.a = (rem << 8) | quot;
            // @TODO: clear flags on undefined instead of ignoring?
        } break;

        // @TODO: check effect and what byte influences the flags
        case e_op_aad:
            write_reg(get_low_byte_reg_access(e_reg_a), (g_machine.a & 0xFF) % 10);
            update_common_flags(g_machine.a & 0xFF, false);
            break;

        case e_op_cbw:
            g_machine.a = (i8)(g_machine.a & 0xFF);
            break;

        case e_op_cwd:
            g_machine.d = sgn(g_machine.a);
            break;

        case e_op_xor: {
            u32 res = op0_val ^ op1_val;
            update_logic_flags(res, w);
            write_operand(op0, res, w, seg_override);
        } break;

        case e_op_test: {
            u32 res = op0_val & op1_val;
            update_logic_flags(res, w);
        } break;

        case e_op_inc: {
            u32 res = op0_val + 1;
            update_arifm_flags(op0_val, 1, w);
            write_operand(op0, res, w, seg_override);
        } break;

        case e_op_dec: {
            u32 res = op0_val - 1;
            update_arifm_flags(op0_val, -1, w);
            write_operand(op0, res, w, seg_override);
        } break;

        case e_op_neg:
            write_operand(op0, -op0_val, w, seg_override);
            break;

        case e_op_not:
            write_operand(op0, ~op0_val, w, seg_override);
            break;

        case e_op_shl: {
            u32 res = op0_val << op1_val;
            write_operand(op0, res, w, seg_override);
            update_shift_flags((op0_val << (op1_val-1)) & hmask(w), res, op0_val, w);
        } break;

        case e_op_shr: {
            u32 res = op0_val >> op1_val;
            write_operand(op0, res, w, seg_override);
            update_shift_flags((op0_val >> (op1_val-1)) & 0x1, res, op0_val, w);
        } break;

        case e_op_sar: {
            i32 res = (i32)op0_val >> op1_val;
            write_operand(op0, res, w, seg_override);
            update_shift_flags(((i32)op0_val >> (op1_val-1)) & 0x1, res, op0_val, w);
        } break;

        case e_op_je:
            metadata.cond_action_happened = cond_jump(op0_val, zf);
            break;
        case e_op_jl:
            metadata.cond_action_happened = cond_jump(op0_val, sf && !zf);
            break;
        case e_op_jle:
            metadata.cond_action_happened = cond_jump(op0_val, sf || zf);
            break;
        case e_op_jb:
            metadata.cond_action_happened = cond_jump(op0_val, cf && !zf);
            break;
        case e_op_jbe:
            metadata.cond_action_happened = cond_jump(op0_val, cf || zf);
            break;
        case e_op_jp:
            metadata.cond_action_happened = cond_jump(op0_val, pf);
            break;
        case e_op_jo:
            metadata.cond_action_happened = cond_jump(op0_val, of);
            break;
        case e_op_js:
            metadata.cond_action_happened = cond_jump(op0_val, sf);
            break;
        case e_op_jne:
            metadata.cond_action_happened = cond_jump(op0_val, !zf);
            break;
        case e_op_jge:
            metadata.cond_action_happened = cond_jump(op0_val, !sf || zf);
            break;
        case e_op_jg:
            metadata.cond_action_happened = cond_jump(op0_val, !sf && !zf);
            break;
        case e_op_jae:
            metadata.cond_action_happened = cond_jump(op0_val, !cf || zf);
            break;
        case e_op_ja:
            metadata.cond_action_happened = cond_jump(op0_val, !cf && !zf);
            break;
        case e_op_jnp:
            metadata.cond_action_happened = cond_jump(op0_val, !pf);
            break;
        case e_op_jno:
            metadata.cond_action_happened = cond_jump(op0_val, !of);
            break;
        case e_op_jns:
            metadata.cond_action_happened = cond_jump(op0_val, !sf);
            break;

        case e_op_loop:
            metadata.cond_action_happened = cx_loop_jump(op0_val, -1);
            break;
        case e_op_loopz:
            metadata.cond_action_happened = cx_loop_jump(op0_val, -1, zf);
            break;
        case e_op_loopnz:
            metadata.cond_action_happened = cx_loop_jump(op0_val, -1, !zf);
            break;
        case e_op_jcxz:
            metadata.cond_action_happened = cx_loop_jump(op0_val, 0);
            break;

        case e_op_in:
        case e_op_out:
            nop_effect();
            break;

        default:
            LOGERR("Instruction execution not implemented");
            assert(0);
    }

    if (g_tracing.flags & e_trace_data_mutation) {
        for (int reg = 0; reg < e_reg_flags; ++reg)
            if (prev_machine.regs[reg] != g_machine.regs[reg]) {
                output::print(" ");
                output::print_word_reg((reg_t)reg);
                output::print(":0x%hx->0x%hx", prev_machine.regs[reg], g_machine.regs[reg]);
            }

        if (prev_machine.flags != g_machine.flags) {
            output::print(" flags:");
            output_flags_content(prev_machine.flags);
            output::print("->");
            output_flags_content(g_machine.flags);
        }
    }
    if (g_tracing.flags & e_trace_cycles) {
        u32 elapsed_cycles = estimate_instruction_clocks(metadata);
        g_tracing.total_cycles += elapsed_cycles;
        output::print(" | Clocks: +%u=%u", elapsed_cycles, g_tracing.total_cycles);
    }

    if (g_tracing.flags)
        output::print("\n");

    return get_full_ip();
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
    output_flags_content(g_machine.flags);
    output::print("\n");
}

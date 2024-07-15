#include "simulator.hpp"
#include "consts.hpp"
#include "memory.hpp"
#include "print.hpp"
#include "util.hpp"
#include "clocks.hpp"
#include "instruction.hpp"
#include <cassert>
#include <cmath>
#include <cstdlib>

// @TODO: think again if asserts are appropriate here.
//        Where are we dealing with caller mistakes, and where with bad input?

// @TODO: warning output when tracing cycles

// @TODO: in-out ports side effect tracing?

// @TODO: get rid of convoluted access calls, further

// @TODO: nop instr?

// @TODO: clear flags that are undefined instead of ignoring them?

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

union reg_mem_t {
    u16 word;
    u8 bytes[2];
};

struct machine_t {
    union {
        struct {
            reg_mem_t a;
            reg_mem_t b;
            reg_mem_t c;
            reg_mem_t d;
            reg_mem_t sp;
            reg_mem_t bp;
            reg_mem_t si;
            reg_mem_t di;
            reg_mem_t es;
            reg_mem_t cs;
            reg_mem_t ss;
            reg_mem_t ds;
            reg_mem_t ip;
            reg_mem_t flags;
        };
        reg_mem_t regs[e_reg_max];
    };
};

struct tracing_state_t {
    u32 flags = 0;
    u32 ip_offset_from_prefixes = 0;
    u32 total_cycles = 0;
};

static machine_t g_machine = {};
static tracing_state_t g_tracing = {};

#define W(reg_)  (reg_).word
#define LO(reg_) (reg_).bytes[is_little_endian() ? 0 : 1]
#define HI(reg_) (reg_).bytes[is_little_endian() ? 1 : 0]

#define WREG(reg_id_)  W(g_machine.regs[reg_id_])
#define LOREG(reg_id_) LO(g_machine.regs[reg_id_])
#define HIREG(reg_id_) HI(g_machine.regs[reg_id_])

#define AL LO(g_machine.a)
#define AH HI(g_machine.a)
#define AX W(g_machine.a)

#define BL LO(g_machine.b)
#define BH HI(g_machine.b)
#define BX W(g_machine.b)

#define CL LO(g_machine.c)
#define CH HI(g_machine.c)
#define CX W(g_machine.c)

#define DL LO(g_machine.d)
#define DH HI(g_machine.d)
#define DX W(g_machine.d)

#define SP    W(g_machine.sp)
#define BP    W(g_machine.bp)
#define SI    W(g_machine.si)
#define DI    W(g_machine.di)
#define CS    W(g_machine.cs)
#define SS    W(g_machine.ss)
#define DS    W(g_machine.ds)
#define ES    W(g_machine.es)
#define IP    W(g_machine.ip)
#define FLAGS W(g_machine.flags)

void set_simulation_trace_option(u32 flags, bool set)
{
    set_flags(&g_tracing.flags, flags, set);
}

static u32 hmask(bool is_wide)
{
    return 1 << (is_wide ? 15 : 7);
}

static u32 max_val(bool is_wide)
{
    return is_wide ? 0xFFFF : 0xFF;
}

static i32 max_ival(bool is_wide)
{
    return is_wide ? 0x7FFF : 0x7F;
}

static bool is_zero(u32 n, bool is_wide)
{
    return (n & max_val(is_wide)) == 0;
};

static bool is_neg(u32 n, bool is_wide)
{
    return n & hmask(is_wide);
};

static bool is_parity(u32 n)
{
    return (count_ones(n, 8) & 1) == 0;
}

static bool is_arifm_carry(u32 n, bool is_wide)
{
    return n & (hmask(is_wide) << 1);
}

static void set_pflag(u16 flag, bool val)
{
    if (val)
        FLAGS |= flag;
    else
        FLAGS &= ~flag;
}

static bool get_pflag(u16 flag)
{
    return FLAGS & flag;
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
        return WREG(access.reg);
    else if (access.offset == 1)
        return HIREG(access.reg);
    else
        return LOREG(access.reg);
}

static void write_reg(reg_access_t access, u16 val)
{
    assert((access.offset == 0 && access.size == 2) ||
           (access.reg <= e_reg_d && access.size == 1 && access.offset <= 1));

    if (access.size == 2) // => offset = 0
        WREG(access.reg) = val;
    else if (access.offset == 1)
        HIREG(access.reg) = val;
    else
        LOREG(access.reg) = val;
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
    seg.base += get_address_in_segment(WREG(seg_reg), 0);
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
    //        Seems that es only pops up in the *s instructions themselves.
    if (access.base == e_ea_base_bp ||
        access.base == e_ea_base_bp_di ||
        access.base == e_ea_base_bp_si)
    {
        return get_segment_access(e_reg_ss);
    } else
        return get_segment_access(e_reg_ds);
}

static void trace_mem_write(u32 addr, u16 prev_content, u16 new_content, bool is_wide)
{
    output::print(" ");
    if (is_wide)
        output::print("[0x%x-0x%x]", addr, addr+1);
    else
        output::print("[0x%x]", addr);
    output::print(":0x%hx->0x%hx", prev_content, new_content);
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
        trace_mem_write(addr, prev_content, read_mem(access, is_wide, seg_override), is_wide);
    }
}

static u32 read_operand(operand_t op, bool is_wide, reg_t seg_override = e_reg_max)
{
    switch (op.type) {
        case e_operand_reg:
            return read_reg(op.data.reg);

        case e_operand_imm:
            return op.data.imm;

        case e_operand_mem:
            return read_mem(op.data.mem, is_wide, seg_override);

        case e_operand_cs_ip:
            return (op.data.cs_ip.cs << 16) | op.data.cs_ip.ip;

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

static u16 pop_from_stack(bool is_wide)
{
    memory_access_t stack = get_segment_access(e_reg_ss);
    u16 val = read_val_at(stack, SP, is_wide);
    SP += is_wide ? 2 : 1;
    return val;
}

static void push_to_stack(u16 val, bool is_wide)
{
    SP -= is_wide ? 2 : 1;
    memory_access_t stack = get_segment_access(e_reg_ss);

    u16 prev_content = read_val_at(stack, SP, is_wide);
    write_val_to(stack, SP, val, is_wide);
    
    if (g_tracing.flags & e_trace_data_mutation)
        trace_mem_write(stack.base + SP, prev_content, val, is_wide); 
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

// @TODO: recheck
static void update_shift_flags(bool pushed_bit, u32 res, u32 orig, bool is_wide)
{
    set_pflag(e_pflag_c, pushed_bit);
    set_pflag(e_pflag_o, is_neg(res, is_wide) != is_neg(orig, is_wide));
}

static void ascii_adjust_addsub(u8 al_adjust, u8 ah_carry_adjust)
{
    if (AL > 9) {
        AL += al_adjust;
        AH += ah_carry_adjust;
        set_pflag(e_pflag_c, true);
        set_pflag(e_pflag_a, true);
    } else {
        set_pflag(e_pflag_c, false);
        set_pflag(e_pflag_a, false);
    }
    
    AL &= 0xF;
}

static void decimal_adjust_addsub(u8 al_adjust, u8 al_hi_adjust)
{
    u8 old_al = AL;
    if ((AL & 0xF) > 9 || get_pflag(e_pflag_a)) {
        AL += al_adjust; // Carries the 1 over on it's own, use pos adjust
        set_pflag(e_pflag_a, true);
    } else
        set_pflag(e_pflag_a, false);
    if (old_al > 0x99) {
        AL += al_hi_adjust;
        set_pflag(e_pflag_c, true);
    } else
        set_pflag(e_pflag_c, false);

    update_common_flags(AL, false);
}

template <class TNum32, class TNum16, class TNum8>
static void do_multiplication(u32 mult, bool is_wide)
{
    static_assert(sizeof(TNum32) == 4, "Pass i32 or u32 here");
    static_assert(sizeof(TNum16) == 2, "Pass i16 or u16 here");
    static_assert(sizeof(TNum8)  == 1, "Pass i8 or u8 here");

    TNum32 adj_mult = is_wide ? (TNum16)mult : (TNum8)mult;
    TNum32 base     = is_wide ? (TNum16)AX : (TNum8)AL;

    TNum32 res = base * adj_mult;
    TNum32 lo;
    if (is_wide) {
        DX = res >> 16;
        AX = res & 0xFFFF;
        lo = AX;
    } else {
        AX = res;
        lo = AL;
    }
    bool carry_overflow = res == lo;
    set_pflag(e_pflag_c, carry_overflow);
    set_pflag(e_pflag_o, carry_overflow);
}

template <class TNum32, class TNum16, class TNum8, class TFQuotChecker>
static void do_division(u32 divisor, bool is_wide, TFQuotChecker &&quot_checker)
{
    static_assert(sizeof(TNum32) == 4, "Pass i32 or u32 here");
    static_assert(sizeof(TNum16) == 2, "Pass i16 or u16 here");
    static_assert(sizeof(TNum8)  == 1, "Pass i8 or u8 here");

    if (divisor == 0) {
        // @TODO: generate int 0 instead, and log it properly
        // And make a test that actually does something useful
        LOGERR("Int 0 exception");
        //exit(1);
        return;
    }

    TNum32 adj_div = is_wide ? (TNum16)divisor : (TNum8)divisor;
    TNum32 base    = is_wide ? ((DX << 16) | AX) : (TNum16)AX;

    TNum32 quot = base / adj_div;
    TNum32 rem  = base % adj_div;

    // @TODO: as above
    if (!quot_checker(quot)) {
        LOGERR("Int 0 exception");
        return;
    }

    if (is_wide) {
        DX = rem;
        AX = quot;
    } else {
        AH = rem;
        AL = quot;
    }
}

static bool cond_jump(u16 disp, bool cond)
{
    if (cond)
        IP += disp;

    return cond;
}

static bool cx_loop_jump(u16 disp, i16 delta_cx, bool cond = true)
{
    CX += delta_cx;

    if (CX != 0 && cond) {
        IP += disp;
        return true;
    }

    return false;
}

static void uncond_jump(operand_t op, bool is_rel, bool is_far, bool save_to_stack)
{
    // @TODO: this whole mess needs validation and cleaning
    if (is_far || op.type == e_operand_cs_ip) {
        u16 cs, ip;
        if (op.type == e_operand_mem) { // => is_far
            // @TODO: pull out somewhere?
            memory_access_t seg_mem = get_segment_access_for_ea(op.data.mem, e_reg_max);
            u32 cs_ip = read_dword_at(seg_mem, calculate_ea(op.data.mem));
            cs = cs_ip >> 16;
            ip = cs_ip & 0xFFFF;
        } else { // e_operand_cs_ip, @TODO: validate
            cs = op.data.cs_ip.cs;
            ip = op.data.cs_ip.ip;
        }

        if (save_to_stack) {
            push_to_stack(CS, true);
            push_to_stack(IP, true);
        }

        CS = cs;
        IP = ip;
    } else { // intrasegment
        u16 disp;
        if (op.type == e_operand_mem)
            disp = read_mem(op.data.mem, true, e_reg_max);
        else
            disp = op.data.imm;

        if (save_to_stack)
            push_to_stack(IP, true);

        if (is_rel)
            IP += disp;
        else
            IP = disp;
    }
}

// @TODO: consts in other places? This here can enable optimizations, I think.
//        check disassembly of this func
static u32 string_instruction(const op_t op, const bool is_wide,
                              const bool rep, const bool req_zero)
{
    const memory_access_t src_base = get_segment_access(e_reg_ds);
    const memory_access_t dst_base = get_segment_access(e_reg_es);
    auto execute_op = [op, is_wide, &src_base, &dst_base]() {
        u16 src_val   = read_val_at(src_base, SI, is_wide);
        u16 dst_val   = read_val_at(dst_base, DI, is_wide);
        u16 accum_val = is_wide ? AX : AL;

        switch (op) {
        case e_op_movs:
            write_val_to(dst_base, DI, src_val, is_wide);
            ++SI;
            ++DI;
            break;

        case e_op_cmps:
            update_arifm_flags(src_val, -dst_val, is_wide);
            ++SI;
            ++DI;
            break;

        case e_op_scas:
            update_arifm_flags(accum_val, -dst_val, is_wide);
            ++DI;
            break;

        case e_op_lods:
            if (is_wide)
                AX = src_val;
            else
                AL = src_val;
            ++SI;
            break;

        case e_op_stos:
            write_val_to(dst_base, DI, accum_val, is_wide);
            ++DI;
            break;

        default:
            LOGERR("Not a valid string instruction"); // @TODO: what instruction
            assert(0);
        }
    };

    if (!rep) {
        execute_op();
        return 1;
    }

    // w/ rep
    u32 repetitions;
    for (repetitions = 0;
         CX != 0 && get_pflag(e_pflag_z) == req_zero;
         --CX, ++repetitions)
    {
        execute_op();
    }

    return repetitions;
}

static u32 get_full_ip()
{
    return get_address_in_segment(CS, IP);
}

u32 get_simulation_ip()
{
    return get_full_ip();
}

u32 simulate_instruction_execution(instruction_t instr)
{
    machine_t prev_machine = g_machine;
#define PREV_WREG(reg_id_) prev_machine.regs[reg_id_].word
#define PREV_IP            prev_machine.ip.word
#define PREV_FLAGS         prev_machine.flags.word

    IP += instr.size;

    if ((instr.op == e_op_ret || instr.op == e_op_retf) &&
        (g_tracing.flags & e_trace_stop_on_ret))
    {
        output::print("STOPONRET: Return encountered at address %u\n", get_full_ip());
        return c_ip_terminate; // exits loop
    }

    if (instr.op == e_op_lock || instr.op == e_op_rep || instr.op == e_op_segment) {
        g_tracing.ip_offset_from_prefixes += instr.size;
        return get_full_ip();
    } else if (g_tracing.ip_offset_from_prefixes > 0) {
        PREV_IP -= g_tracing.ip_offset_from_prefixes;
        g_tracing.ip_offset_from_prefixes = 0;
    }

    // @TODO: s flag for add is already needed?
    const bool w   = instr.flags & e_iflags_w;
    const bool z   = instr.flags & e_iflags_z;
    const bool rep = instr.flags & e_iflags_rep;

    const bool rel_disp = instr.flags & e_iflags_imm_is_rel_disp;
    const bool far      = instr.flags & e_iflags_far;

    int op_bytes = w ? 2 : 1;
    int op_bits  = op_bytes * 8;
    int op_mask  = n_bit_mask(op_bits);

    reg_t seg_override = (instr.flags & e_iflags_seg_override) ?
                         instr.segment_override : e_reg_max;

    int op_cnt    = instr.operand_cnt;
    operand_t op0 = instr.operands[0];
    operand_t op1 = instr.operands[1];
    u32 op0_val = op0.type == e_operand_none ? 0 : read_operand(op0, w, seg_override);
    u32 op1_val = op1.type == e_operand_none ? 0 : read_operand(op1, w, seg_override);

    instruction_metadata_t metadata = {};
    metadata.instr = instr;
    metadata.op0_val = op0_val;
    metadata.op1_val = op1_val;

    const bool zf = get_pflag(e_pflag_z);
    const bool sf = get_pflag(e_pflag_s);
    const bool pf = get_pflag(e_pflag_p);
    const bool of = get_pflag(e_pflag_o);
    const bool cf = get_pflag(e_pflag_c);
    const bool af = get_pflag(e_pflag_a);

    if (g_tracing.flags & e_trace_disassembly)
        output::print_instruction(instr);
    if (g_tracing.flags & (e_trace_data_mutation | e_trace_cycles))
        output::print(" ;");

    // @TODO: correct instruction format validation
    // (as a separate module, it could be useful in decoder/main loop,
    //  since the whole system relies on this invariant)

    switch (instr.op) {
        case e_op_mov:
            write_operand(op0, op1_val, w, seg_override);
            break;

        case e_op_push:
            push_to_stack(op0_val, w);
            break;

        case e_op_pop:
           // @TODO: restrict popping cs
            write_operand(op0, pop_from_stack(w), w);
            break;

        case e_op_xchg:
            write_operand(op0, op1_val, w, seg_override);
            write_operand(op1, op0_val, w, seg_override);
            break;

        case e_op_xlat: {
            ea_mem_access_t access = {e_ea_base_bx, AL};
            AL = read_mem(access, false, e_reg_max);
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
            (instr.op == e_op_lds ? DS : ES) = base;
        } break;

        case e_op_lahf:
            write_reg(get_high_byte_reg_access(e_reg_a),
                      FLAGS & (e_pflag_s | e_pflag_z | e_pflag_a | e_pflag_p | e_pflag_c));
            break;

        case e_op_sahf: {
            set_pflag(e_pflag_s, AH & e_pflag_s);
            set_pflag(e_pflag_z, AH & e_pflag_z);
            set_pflag(e_pflag_a, AH & e_pflag_a);
            set_pflag(e_pflag_p, AH & e_pflag_p);
            set_pflag(e_pflag_c, AH & e_pflag_c);
        } break;

        // @TODO: pull out stach operations
        case e_op_pushf:
            push_to_stack(FLAGS, true);
            break;

        case e_op_popf:
            FLAGS = pop_from_stack(true);
            break;

        case e_op_adc:
            if (get_pflag(e_pflag_c))
                ++op1_val;
        case e_op_add:
            update_arifm_flags(op0_val, op1_val, w);
            write_operand(op0, op0_val + op1_val, w, seg_override);
            break;

        case e_op_aaa:
            ascii_adjust_addsub(6, 1);
            break;
        case e_op_aas:
            ascii_adjust_addsub(-6, -1);
            break;

        case e_op_daa:
            decimal_adjust_addsub(6, 0x60);
            break;
        case e_op_das:
            decimal_adjust_addsub(-6, -0x60);
            break;

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

        case e_op_mul:
            do_multiplication<u32, u16, u8>(op0_val, w);
            break;
        case e_op_imul:
            do_multiplication<i32, i16, i8>(op0_val, w);
            break;

        case e_op_div:
            do_division<u32, u16, u8>(
                op0_val, w, [w](u32 quot) { return quot <= max_val(w); });
            break;
        case e_op_idiv:
            do_division<i32, i16, i8>(
                op0_val, w, [w](u32 quot) { return (i32)quot <= max_ival(w) && (i32)quot >= -max_ival(w); });
            break;

        case e_op_aam:
            AH = AL / 10;
            AL -= AH * 10;
            update_common_flags(AL, false);
            break;

        case e_op_aad:
            AX = (AH*10 + AL) & 0xFF;
            update_common_flags(AL, false);
            break;

        case e_op_cbw:
            AX = (i8)AL;
            break;

        case e_op_cwd:
            DX = sgn(AX);
            break;

        case e_op_and: {
            u32 res = op0_val & op1_val;
            write_operand(op0, res, w, seg_override);
            update_logic_flags(res, w);
        } break;

        case e_op_test: {
            u32 res = op0_val & op1_val;
            update_logic_flags(res, w);
        } break;

        case e_op_or: {
            u32 res = op0_val | op1_val;
            write_operand(op0, res, w, seg_override);
            update_logic_flags(res, w);
        } break;

        case e_op_xor: {
            u32 res = op0_val ^ op1_val;
            update_logic_flags(res, w);
            write_operand(op0, res, w, seg_override);
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
            update_common_flags(res, w); // @TODO: verify with Casey's listing
        } break;

        case e_op_shr: {
            u32 res = op0_val >> op1_val;
            write_operand(op0, res, w, seg_override);
            update_shift_flags((op0_val >> (op1_val-1)) & 0x1, res, op0_val, w);
            update_common_flags(res, w);
        } break;

        case e_op_sar: {
            i32 res = (i32)op0_val >> op1_val;
            write_operand(op0, res, w, seg_override);
            update_shift_flags(((i32)op0_val >> (op1_val-1)) & 0x1, res, op0_val, w);
            update_common_flags(res, w);
        } break;

        // @TODO: pull out common logic
        // @TODO: check that all rotates also use sign chage for OF.
        //        Also, check the claim about OF being set only on count=1.
        case e_op_rcl: // @HACK: put the CF bit into the op
            op0_val |= get_pflag(e_pflag_o) << op_bits;
            ++op_bits;
        case e_op_rol: {
            int rot_bits = op1_val & op_bits; // 8 or 16 is a valid mask
            int left_bits = op_bits - rot_bits;
            u32 res = (op0_val << rot_bits) | (op0_val >> left_bits);
            write_operand(op0, res & op_mask, w, seg_override);
            update_shift_flags(res & 1, res & op_mask, op0_val & op_mask, w);
        } break;

        case e_op_rcr: // @HACK: put the CF bit into the op
            op0_val |= get_pflag(e_pflag_o) << op_bits;
            ++op_bits;
        case e_op_ror: {
            int rot_bits = op1_val & op_bits; // 8 or 16 is a valid mask
            int left_bits = op_bits - rot_bits;
            u16 res = (op0_val >> rot_bits) | (op0_val << left_bits);
            write_operand(op0, res & op_mask, w, seg_override);
            update_shift_flags(res & hmask(w), res & op_mask, op0_val & op_mask, w);
        } break;

        case e_op_movs:
        case e_op_cmps:
        case e_op_scas:
        case e_op_lods:
        case e_op_stos:
            metadata.rep_count = string_instruction(instr.op, w, rep, z);
            break;

        case e_op_call:
            uncond_jump(op0, rel_disp, far, true);
            break;
        case e_op_jmp:
            uncond_jump(op0, rel_disp, far, false);
            break;

        case e_op_retf:
            IP = pop_from_stack(true);
            CS = pop_from_stack(true);
            if (op_cnt) IP += op0_val;
            break;

        case e_op_ret:
            IP = pop_from_stack(true);
            if (op_cnt) IP += op0_val;
            break;

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
            // @TODO: optional keyboard (or even file?) input.
            if (g_tracing.flags & e_trace_data_mutation)
                output::print(" <read %s from port %u>", w ? "word" : "byte", op1_val);
            write_operand(op0, 0, w); // @TEMP
            break;
            
        case e_op_out:
            if (g_tracing.flags & e_trace_data_mutation)
                output::print(" <write %s 0x%hx to port %hu>", w ? "word" : "byte", op1_val, op0_val);
            break;

        // @NOTE: this is not really accurate, but suffices for our simulation
        case e_op_hlt:
            output::print("\n");
            return c_ip_terminate;

        default:
            LOGERR("Instruction execution not implemented"); // @TODO: what instruction?
            assert(0);
    }

    if (g_tracing.flags & e_trace_data_mutation) {
        for (int reg = 0; reg < e_reg_flags; ++reg)
            if (PREV_WREG(reg) != WREG(reg)) {
                output::print(" ");
                output::print_word_reg((reg_t)reg);
                output::print(":0x%hx->0x%hx", PREV_WREG(reg), WREG(reg));
            }

        if (PREV_FLAGS != FLAGS) {
            output::print(" flags:");
            output_flags_content(PREV_FLAGS);
            output::print("->");
            output_flags_content(FLAGS);
        }
    }
    if (g_tracing.flags & e_trace_cycles) {
        u32 elapsed_cycles = estimate_instruction_clocks(metadata);
        g_tracing.total_cycles += elapsed_cycles;
        output::print(" | Clocks: +%u=%u", elapsed_cycles, g_tracing.total_cycles);
    }

    if (g_tracing.flags)
        output::print("\n");

#undef PREV_WREG
#undef PREV_IP
#undef PREV_FLAGS

    return get_full_ip();
}

void output_simulation_results()
{
    if (g_tracing.flags != 0)
        output::print("\n");
    output::print("Registers state:\n");
    for (int reg = e_reg_a; reg < e_reg_flags; ++reg) {
        u16 val = read_reg(get_word_reg_access((reg_t)reg));
        output::print("      ");
        output::print_word_reg((reg_t)reg);
        output::print(": 0x%04hx (%hu)\n", val, val);
    }
    output::print("   flags: ");
    output_flags_content(FLAGS);
    if (g_tracing.flags & e_trace_cycles)
        output::print("\n\nTotal clocks: %d", g_tracing.total_cycles);
    output::print("\n");
}

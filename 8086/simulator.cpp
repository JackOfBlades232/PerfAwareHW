#include "simulator.hpp"
#include "consts.hpp"
#include "defs.hpp"
#include "memory.hpp"
#include "print.hpp"
#include "util.hpp"
#include "clocks.hpp"
#include "instruction.hpp"
#include "input.hpp"
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <thread>

// @SPEED: check if making as much stuff as possible const will speed things up

using namespace std::chrono_literals;

static constexpr u32 c_seg_size = POT(16);
static constexpr u32 c_no_exception = -1;

enum proc_flag_t {
    e_pflag_c = 1 << 0,
    e_pflag_p = 1 << 2,
    e_pflag_a = 1 << 4,
    e_pflag_z = 1 << 6,
    e_pflag_s = 1 << 7,
    e_pflag_d = 1 << 8,
    e_pflag_i = 1 << 9,
    e_pflag_t = 1 << 10,
    e_pflag_o = 1 << 11,
};

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

struct simulation_context_t {
    instruction_metadata_t *rec_metadata;
    u32 thrown_exception = c_no_exception;
};

struct tracing_state_t {
    u32 flags = 0;
    u32 ip_offset_from_prefixes = 0;
    u32 total_cycles = 0;
};

static machine_t g_machine        = {};
static simulation_context_t g_ctx = {};
static tracing_state_t g_tracing  = {};

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

void output_simulation_disclaimer()
{
    output::print(
        "\nDISCLAIMER: this is not a professional-grade simulator.\n"
        "            The simulator follows the published manual, thus results may"
        "differ from actual 8086/8088, or from 16bit mode on modern processors.\n"
        "            Furthermore, some instructions are not simulated correctly on"
        "purpose, namely all the ones that interact with external hardware.\n\n");

    if (g_tracing.flags & e_trace_cycles) {
        output::print(
            "The cycles estimations also follow the manual rather than real benchmarks."
            "Also, for instructions with ranged estimation the high limit is used.\n"
            "And, the manual definitely has typos, thus the estimations can only be"
            "used for relative comparisons in some cases.\n\n");
    }
}

static u32 bytes(bool is_wide)
{
    return is_wide ? 2 : 1;
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

static void clear_undefined_flags(u16 flags)
{
    set_pflag(flags, false);
}

static void output_flags_content(u16 flags_val)
{
    proc_flag_t flags[] =
    {
        e_pflag_c, e_pflag_p, e_pflag_a, e_pflag_z,
        e_pflag_s, e_pflag_t, e_pflag_i, e_pflag_d,
        e_pflag_o
    };
    const char *flags_names[ARR_CNT(flags)] =
    {
        "C", "P", "A", "Z", "S",
        "T", "I", "D", "O"
    };
    for (int f = 0; f < ARR_CNT(flags); ++f) {
        if (flags_val & flags[f])
            output::print("%s", flags_names[f]);
    }
}

static void record_wide_access(memory_access_t at, u32 offset)
{
    ++g_ctx.rec_metadata->wide_transfer_cnt;
    if ((at.base + offset) & 0x1) // odd
        ++g_ctx.rec_metadata->wide_odd_transfer_cnt;
}

// @NOTE: used as a helper function to get a value, without being a machine read
static u16 read_val_at_non_simulated(memory_access_t at, u32 offset, bool is_wide)
{
    if (is_wide)
        return read_word_at(at, offset);
    else
        return read_byte_at(at, offset);
}

static u16 read_val_at(memory_access_t at, u32 offset, bool is_wide)
{
    if (is_wide) {
        record_wide_access(at, offset);
        return read_word_at(at, offset);
    } else
        return read_byte_at(at, offset);
}

static void write_val_to(memory_access_t to, u32 offset, u32 val, bool is_wide)
{
    if (is_wide) {
        record_wide_access(to, offset);
        write_word_to(to, offset, val);
    } else
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
    u16 op1 = r1 == c_null_reg ? 0 : WREG(r1);
    u16 op2 = r2 == c_null_reg ? 0 : WREG(r2);
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

static memory_access_t get_segment_access_for_ea(ea_mem_access_t access, reg_t seg_override)
{
    if (seg_override != e_reg_max) {
        assert(seg_override == e_reg_cs ||
               seg_override == e_reg_ss ||
               seg_override == e_reg_ds ||
               seg_override == e_reg_es);
        return get_segment_access(seg_override);
    }

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

    u16 prev_content = read_val_at_non_simulated(seg_mem, offset, is_wide);

    write_val_to(seg_mem, offset, val, is_wide);

    if (g_tracing.flags & e_trace_data_mutation) {
        u32 addr = get_full_address(seg_mem, offset);
        trace_mem_write(addr, prev_content, val, is_wide);
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
            ASSERTF(0, "Can't write to immediate operand");
        case e_operand_cs_ip:
            ASSERTF(0, "Can't write to cs/ip operand");

        case e_operand_none:
            assert(0);
    }
}

static void throw_exception(u8 ex)
{
    g_ctx.thrown_exception = ex;
}

static void process_exceptions()
{
    if (g_ctx.thrown_exception == c_no_exception)
        return;

    u32 saved_tracing_flags = g_tracing.flags;
    {
        set_flags(&g_tracing.flags, e_trace_disassembly, false);
        instruction_t int_instr{
            .op          = e_op_int,
            .size        = 0, // This should not affect IP
            .operands    = {get_imm_operand((u8)g_ctx.thrown_exception)},
            .operand_cnt = 1
        };

        output::print("<exception interrupt of type %u generated>\n", g_ctx.thrown_exception);

        // Do it before call to simulate so as not to recurse
        g_ctx.thrown_exception = c_no_exception;

        simulate_instruction_execution(int_instr);
    }
    g_tracing.flags = saved_tracing_flags; 
}

static u16 pop_from_stack(bool is_wide)
{
    memory_access_t stack = get_segment_access(e_reg_ss);
    u16 val = read_val_at(stack, SP, is_wide);
    SP += bytes(is_wide);
    return val;
}

static void push_to_stack(u16 val, bool is_wide)
{
    SP -= bytes(is_wide);
    memory_access_t stack = get_segment_access(e_reg_ss);

    u16 prev_content = read_val_at_non_simulated(stack, SP, is_wide);
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

    clear_undefined_flags(e_pflag_a);
}

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

    clear_undefined_flags(e_pflag_o | e_pflag_s | e_pflag_z | e_pflag_p);
    
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
    clear_undefined_flags(e_pflag_o);
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

    clear_undefined_flags(e_pflag_s | e_pflag_z | e_pflag_a | e_pflag_p);
}

template <class TNum32, class TNum16, class TNum8, class TFQuotChecker>
static void do_division(u32 divisor, bool is_wide, TFQuotChecker &&quot_checker)
{
    static_assert(sizeof(TNum32) == 4, "Pass i32 or u32 here");
    static_assert(sizeof(TNum16) == 2, "Pass i16 or u16 here");
    static_assert(sizeof(TNum8)  == 1, "Pass i8 or u8 here");

    if (divisor == 0) {
        throw_exception(0);
        return;
    }

    TNum32 adj_div = is_wide ? (TNum16)divisor : (TNum8)divisor;
    TNum32 base    = is_wide ? ((DX << 16) | AX) : (TNum16)AX;

    TNum32 quot = base / adj_div;
    TNum32 rem  = base % adj_div;

    if (!quot_checker(quot)) {
        throw_exception(0);
        return;
    }

    if (is_wide) {
        DX = rem;
        AX = quot;
    } else {
        AH = rem;
        AL = quot;
    }

    clear_undefined_flags(e_pflag_o | e_pflag_s | e_pflag_z |
                          e_pflag_a | e_pflag_p | e_pflag_c);
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

static void uncond_jump(operand_t op, bool is_rel, bool is_far,
                        bool save_to_stack, reg_t seg_override = e_reg_max)
{
    if (is_far || op.type == e_operand_cs_ip) {
        if (save_to_stack) {
            push_to_stack(CS, true);
            push_to_stack(IP, true);
        }

        ASSERTF(!is_rel, "Wrong instr for intersegment jump, fix validation");

        if (op.type == e_operand_mem) { // => is_far
            ASSERTF(is_far, "Wrong instr for intersegment jump, fix validation");

            memory_access_t seg_mem = get_segment_access_for_ea(op.data.mem, seg_override);
            u32 cs_ip = read_dword_at(seg_mem, calculate_ea(op.data.mem));
            CS = cs_ip >> 16;
            IP = cs_ip & 0xFFFF;
        } else if (op.type == e_operand_cs_ip) {
            CS = op.data.cs_ip.cs;
            IP = op.data.cs_ip.ip;
        } else
            ASSERTF(0, "Wrong instr for intersegment jump, fix validation");
    } else { // intrasegment
        if (save_to_stack)
            push_to_stack(IP, true);

        u16 disp;
        if (op.type == e_operand_mem)
            disp = read_mem(op.data.mem, true, seg_override);
        else if (op.type == e_operand_imm)
            disp = op.data.imm;
        else
            ASSERTF(0, "Wrong instr for intrasegment jump, fix validation");

        if (is_rel)
            IP += disp;
        else
            IP = disp;
    }
}

static u32 string_instruction(op_t op, bool is_wide,
                              bool rep, bool req_zero)
{
    const memory_access_t src_base = get_segment_access(e_reg_ds);
    const memory_access_t dst_base = get_segment_access(e_reg_es);

     auto execute_op = [op, is_wide, req_zero, &src_base, &dst_base]() -> bool {
        u16 increment = bytes(is_wide);

        u16 pre_write_val = read_val_at_non_simulated(dst_base, DI, is_wide);
        u16 written_val   = 0;

        u16 accum_val = is_wide ? AX : AL;

        switch (op) {
        case e_op_movs: {
            u16 src_val = read_val_at(src_base, SI, is_wide);
            write_val_to(dst_base, DI, src_val, is_wide);
            written_val = src_val;
            SI += increment;
            DI += increment;
        } break;

        case e_op_cmps: {
            u16 src_val = read_val_at(src_base, SI, is_wide);
            u16 dst_val = read_val_at(dst_base, DI, is_wide);
            update_arifm_flags(src_val, -dst_val, is_wide);
            SI += increment;
            DI += increment;
        } break;

        case e_op_scas: {
            u16 dst_val = read_val_at(dst_base, DI, is_wide);
            update_arifm_flags(accum_val, -dst_val, is_wide);
            DI += increment;
        } break;

        case e_op_lods: {
            u16 src_val = read_val_at(src_base, SI, is_wide);
            if (is_wide)
                AX = src_val;
            else
                AL = src_val;
            SI += increment;
        } break;

        case e_op_stos:
            write_val_to(dst_base, DI, accum_val, is_wide);
            written_val = accum_val;
            DI += increment;
            break;

        default:
            ASSERTF(0, "Not a valid string instruction, fix validation");
        }

        if (g_tracing.flags & e_trace_data_mutation) {
            if (op == e_op_stos || op == e_op_movs) {
                trace_mem_write(dst_base.base + DI - 1,
                                pre_write_val,
                                written_val,
                                is_wide);
            }
        }

        if (op == e_op_scas || op == e_op_cmps)
            return req_zero == get_pflag(e_pflag_z);
        else
            return true;
    };

    if (!rep) {
        execute_op();
        return 1;
    }

    // w/ rep
    u32 repetitions = 0;
    while (CX) {
        bool keep_going = execute_op();
        --CX;
        if (!keep_going)
            break;
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
    if ((instr.op == e_op_ret || instr.op == e_op_retf) &&
        (g_tracing.flags & e_trace_stop_on_ret))
    {
        output::print("STOPONRET: Return encountered at address %u\n", get_full_ip());
        return c_ip_terminate; // exits loop
    }

    machine_t prev_machine = g_machine;
#define PREV_WREG(reg_id_) prev_machine.regs[reg_id_].word
#define PREV_IP            prev_machine.ip.word
#define PREV_FLAGS         prev_machine.flags.word

    instruction_metadata_t metadata = {};
    metadata.instr = instr;
    
    g_ctx.rec_metadata = &metadata;
    DEFER([]() { g_ctx.rec_metadata = nullptr; });

    IP += instr.size;

    if (instr.op == e_op_lock || instr.op == e_op_rep || instr.op == e_op_segment) {
        g_tracing.ip_offset_from_prefixes += instr.size;
        return get_full_ip();
    } else if (g_tracing.ip_offset_from_prefixes > 0) {
        PREV_IP -= g_tracing.ip_offset_from_prefixes;
        g_tracing.ip_offset_from_prefixes = 0;
    }

    const bool w    = instr.flags & e_iflags_w;
    const bool z    = instr.flags & e_iflags_z;
    const bool rep  = instr.flags & e_iflags_rep;
    const bool lock = instr.flags & e_iflags_lock;

    const bool rel_disp = instr.flags & e_iflags_imm_is_rel_disp;
    const bool far      = instr.flags & e_iflags_far;

    int op_bytes = bytes(w);
    int op_bits  = op_bytes * 8;
    int op_mask  = n_bit_mask(op_bits);

    reg_t seg_override = (instr.flags & e_iflags_seg_override) ?
                         instr.segment_override : e_reg_max;

    int op_cnt    = instr.operand_cnt;
    operand_t op0 = instr.operands[0];
    operand_t op1 = instr.operands[1];

    // @NOTE: in mov don't read the first operand so as not to record access
    u32 op0_val =
        (op0.type == e_operand_none || instr.op == e_op_mov) ? 0 : read_operand(op0, w, seg_override);
    // @NOTE: for lea, don't read the ea operand so as not to record an access
    u32 op1_val =
        (op1.type == e_operand_none || instr.op == e_op_lea) ? 0 : read_operand(op1, w, seg_override);

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

    if (g_tracing.flags & e_trace_data_mutation) {
        if (lock)
            output::print(" <assert bus lock>");
    }

    switch (instr.op) {
    case e_op_mov:
        write_operand(op0, op1_val, w, seg_override);
        break;

    case e_op_push:
        push_to_stack(op0_val, w);
        break;

    case e_op_pop:
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
        clear_undefined_flags(e_pflag_o | e_pflag_a | e_pflag_c);
        break;

    case e_op_aad:
        AX = (AH*10 + AL) & 0xFF;
        update_common_flags(AL, false);
        clear_undefined_flags(e_pflag_o | e_pflag_a | e_pflag_c);
        break;

    case e_op_cbw:
        AX = (i8)AL;
        break;

    case e_op_cwd:
        DX = sgn<i16>(AX);
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
        u32 pre_res = op0_val << (op1_val-1);
        u32 res = pre_res << 1;
        write_operand(op0, res, w, seg_override);
        update_shift_flags(pre_res & hmask(w), res, op0_val, w);
        update_common_flags(res, w);
    } break;

    case e_op_shr: {
        u32 pre_res = op0_val >> (op1_val-1);
        u32 res = pre_res >> 1;
        write_operand(op0, res, w, seg_override);
        update_shift_flags(pre_res & 0x1, res, op0_val, w);
        update_common_flags(res, w);
    } break;

    case e_op_sar: {
        u32 dec_shift = op1_val-1;
        i32 pre_res =
            w ? ((i16)op0_val >> dec_shift) : ((i8)op0_val >> dec_shift);
        i32 res = (w ? (i16)pre_res : (i8)pre_res) >> 1;
        write_operand(op0, res, w, seg_override);
        update_shift_flags(pre_res & 0x1, res, op0_val, w);
        update_common_flags(res, w);
    } break;

    case e_op_rcl:
        op0_val |= get_pflag(e_pflag_c) << op_bits;
        ++op_bits;
    case e_op_rol: {
        int rot_bits = op1_val % op_bits; // Not mask cause can be 9/17
        int left_bits = op_bits - rot_bits;
        u32 res = (op0_val << rot_bits) | (op0_val >> left_bits);
        write_operand(op0, res & op_mask, w, seg_override);
        update_shift_flags(res & 1, res & op_mask, op0_val & op_mask, w);
    } break;

    case e_op_rcr:
        op0_val |= get_pflag(e_pflag_c) << op_bits;
        ++op_bits;
    case e_op_ror: {
        int rot_bits = op1_val % op_bits; // Not mask cause can be 9/17
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

    
    case e_op_int3:
    case e_op_into:
    case e_op_int: {
        if (instr.op == e_op_int3)
            op0_val = 3;
        else if (instr.op == e_op_into) {
            if (get_pflag(e_pflag_o))
                metadata.cond_action_happened = true;
            else
                break;
        }

        push_to_stack(FLAGS, true);
        uncond_jump(get_mem_operand(e_ea_base_direct, op0_val << 2),
                    false, true, true, e_reg_cs);
    } break;

    case e_op_iret:
        IP    = pop_from_stack(true);
        CS    = pop_from_stack(true);
        FLAGS = pop_from_stack(true);
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

    case e_op_clc:
        set_pflag(e_pflag_c, false);
        break;
    case e_op_stc:
        set_pflag(e_pflag_c, true);
        break;
    case e_op_cmc:
        set_pflag(e_pflag_c, !get_pflag(e_pflag_c));
        break;
    case e_op_cld:
        set_pflag(e_pflag_d, false);
        break;
    case e_op_std:
        set_pflag(e_pflag_d, true);
        break;
    case e_op_cli:
        set_pflag(e_pflag_i, false);
        break;
    case e_op_sti:
        set_pflag(e_pflag_i, true);
        break;

    case e_op_in:
        // @FEAT: could be done from keyboard in interactive mode
        if (g_tracing.flags & e_trace_data_mutation)
            output::print(" <read %s from port %u>", w ? "word" : "byte", op1_val);
        write_operand(op0, 0, w);
        break;
        
    case e_op_out:
        if (g_tracing.flags & e_trace_data_mutation)
            output::print(" <write %s 0x%hx to port %hu>", w ? "word" : "byte", op1_val, op0_val);
        break;

    // @NOTE: this is not really accurate, but suffices for our simulation
    case e_op_hlt:
        output::print("\n");
        return c_ip_terminate;

    case e_op_wait: // This is also for shits and giggles
        if (input::interactivity_enabled()) {
            for (int i = 0; i < 6; ++i) {
                for (int j = 0; j < 10; ++j)
                    std::this_thread::sleep_for(0.025s);

                metadata.wait_n += 250000;
                output::print("\n...");
            }
            input::wait_for_lf();
            output::print("\n");
        }
        break;

    case e_op_esc:
        if (g_tracing.flags & e_trace_data_mutation)
            output::print(" <escape to ext processor w/ opcode 0x%hx and operand 0x%hx>", op0_val, op1_val);
        break;

    case e_op_nop:
        break;

    default:
        ASSERTF(0, "Instruction %s execution not implemented",
                output::get_op_mnemonic(instr.op));
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
        proc_type_t proc =
            (g_tracing.flags & e_trace_8088cycles) ? e_proc8088 : e_proc8086;

        u32 elapsed_cycles = estimate_instruction_clocks(metadata, proc);
        g_tracing.total_cycles += elapsed_cycles;
        output::print(" | Clocks: +%u=%u", elapsed_cycles, g_tracing.total_cycles);
    }

    if (g_tracing.flags)
        output::print("\n");

    process_exceptions();

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
    if (g_tracing.flags & e_trace_cycles) {
        const char *proc_name =
            (g_tracing.flags & e_trace_8088cycles) ? "8088" : "8086";
        output::print("\n\nTotal clocks (%s): %d", proc_name, g_tracing.total_cycles);
    }
    output::print("\n");
}

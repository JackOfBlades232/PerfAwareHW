#include "simulator.hpp"
#include "print.hpp"
#include "util.hpp"
#include "instruction.hpp"
#include <cassert>

// @TODO: think again if asserts are appropriate here.
//        Where are we dealing with caller mistakes, and where with bad input?

struct machine_t {
    u16 registers[e_reg_max];
};

struct tracing_state_t {
    u32 flags = 0;
    u32 registers_used = 0;
};

static machine_t g_machine = {};
static tracing_state_t g_tracing = {};

void set_simulation_trace_level(u32 flags)
{
    g_tracing.flags = flags;
}

static u16 read_reg(reg_access_t access)
{
    assert((access.offset == 0 && access.size == 2) ||
           (access.reg <= e_reg_d && access.size == 1 && access.offset <= 1));

    u16 reg_val = g_machine.registers[access.reg];
    if (access.size == 2) // => offset = 0
        return reg_val;
    else if (access.offset == 0) // => size == 1
        return reg_val & 0xFF;
    else // => offset == 1
        return reg_val >> 8;
}

static void write_reg(reg_access_t access, u16 val)
{
    assert((access.offset == 0 && access.size == 2) ||
           (access.reg <= e_reg_d && access.size == 1 && access.offset <= 1));
    assert(access.size == 2 || val < (1 << 8));

    g_tracing.registers_used |= to_flag(access.reg);

    u16 *reg_ptr = &g_machine.registers[access.reg];
    u16 prev_reg_content = *reg_ptr;

    if (access.size == 2) // => offset = 0
        *reg_ptr = val;
    else if (access.offset == 0) { // => size == 1
        *reg_ptr &= 0xFF00;
        *reg_ptr |= val;
    } else { // => offset == 1
        *reg_ptr &= 0x00FF;
        *reg_ptr |= val << 8;
    }

    // @TODO: more elegant/general side-effect tracing. No ideas for design now
    if (g_tracing.flags & e_trace_data_mutation) {
        output::print(" ; ");
        output::print_word_reg(access.reg);
        output::print(":0x%hx->0x%hx\n", prev_reg_content, *reg_ptr);
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

void simulate_instruction_execution(instruction_t instr)
{
    if (g_tracing.flags & e_trace_disassembly)
        output::print_intstruction(instr);

    // @TODO: correct instruction format validation
    switch (instr.op) {
        case e_op_mov:
            write_operand(instr.operands[0], read_operand(instr.operands[1]));
            break;

        default:
            LOGERR("Instruction execution not implemented");
            assert(0);
    }
}

void output_simulation_results()
{
    if (g_tracing.flags != 0)
        output::print("\n");
    output::print("Registers state:\n");
    for (int reg = e_reg_a; reg < e_reg_ip; ++reg) {
        if (g_tracing.registers_used & to_flag(reg)) {
            u16 val = read_reg(get_word_reg_access((reg_t)reg));
            output::print("      ");
            output::print_word_reg((reg_t)reg);
            output::print(": 0x%04hx (%hd)\n", val, val);
        }
    }
}

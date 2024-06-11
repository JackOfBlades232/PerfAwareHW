#include "simulator.hpp"
#include "print.hpp"
#include "util.hpp"
#include "instruction.hpp"
#include <cassert>

// @TODO: trace enabled/disabled

static u32 g_tracing_flags = 0;

// @TODO: align messages
// @TODO: four digits on final regs

// @TODO: concise machine state

static u16 g_registers_state[e_reg_max];

void set_simulation_trace_level(u32 flags)
{
    g_tracing_flags = flags;
}

// @TODO: instruction execution design
//        (anything better than just funcs for all?)

static u16 read_reg(reg_access_t access)
{
    assert((access.offset == 0 && access.size == 2) ||
           (access.reg <= e_reg_d && access.size == 1 && access.offset <= 1));

    u16 reg_val = g_registers_state[access.reg];
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

    // @TODO: more elegant/general side-effect tracing. No ideas for design now
    if (g_tracing_flags & e_trace_data_mutation) {
        output::print(" ; ");
        output::print_word_reg(access.reg);
        output::print(":0x%hx->0x%hx\n", read_reg(access), val);
    }

    u16 *reg_ptr = &g_registers_state[access.reg];
    if (access.size == 2) // => offset = 0
        *reg_ptr = val;
    else if (access.offset == 0) { // => size == 1
        *reg_ptr &= 0xFF00;
        *reg_ptr |= val;
    } else { // => offset == 1
        *reg_ptr &= 0x00FF;
        *reg_ptr |= val << 8;
    }
}

// @TODO: read/write at operand => refactor

void simulate_instruction_execution(instruction_t instr)
{
    if (g_tracing_flags & e_trace_disassembly)
        output::print_intstruction(instr);

    // @TODO: correct instruction format validation
    switch (instr.op) {
        case e_op_mov: {
            u16 val;

            // src
            switch (instr.operands[1].type) {
                case e_operand_reg:
                    val = read_reg(instr.operands[1].data.reg);
                    break;

                case e_operand_imm:
                    val = instr.operands[1].data.imm;
                    break;

                default: 
                    LOGERR("Instruction execution not implemented for these operands");
                    assert(0);
            }

            // dst
            switch (instr.operands[0].type) {
                case e_operand_reg:
                    write_reg(instr.operands[0].data.reg, val);
                    break;        

                default: 
                    LOGERR("Instruction execution not implemented for these operands");
                    assert(0);
            }
        } break;

        default:
            LOGERR("Instruction execution not implemented");
            assert(0);
    }
}

void output_simulation_results()
{
    if (g_tracing_flags != 0)
        output::print("\n");
    output::print("Registers state:\n");
    for (int reg = e_reg_a; reg < e_reg_max; ++reg) {
        output::print_word_reg((reg_t)reg);
        output::print(": 0x%hx\n", read_reg({(reg_t)reg, 0, 2}));
    }
}

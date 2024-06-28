#pragma once
#include "defs.hpp"
#include "instruction.hpp"

enum simulation_trace_bits_t : u32 {
    e_trace_data_mutation = 1 << 1,
    e_trace_disassembly   = 1 << 2,
    e_trace_cycles        = 1 << 3,

    e_trace_stop_on_ret   = 1 << 4
};
void set_simulation_trace_option(u32 flags, bool set_true);

u32 simulate_instruction_execution(instruction_t instr);
void output_simulation_results();

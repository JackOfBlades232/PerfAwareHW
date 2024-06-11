#pragma once
#include "defs.hpp"
#include "instruction.hpp"

enum simulation_trace_bits_t : u32 {
    e_trace_data_mutation = 1 << 1,
    e_trace_disassembly   = 1 << 2,
};
void set_simulation_trace_level(u32 flags);

void simulate_instruction_execution(instruction_t instr);
void output_simulation_results();

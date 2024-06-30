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

struct instruction_metadata_t {
    instruction_t instr;
    u32 op0_val, op1_val;
    bool cond_action_happened;
};

u32 get_simulation_ip();
u32 simulate_instruction_execution(instruction_t instr);
void output_simulation_results();

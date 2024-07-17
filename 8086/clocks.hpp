#pragma once
#include "defs.hpp"
#include "instruction.hpp"

struct instruction_metadata_t {
    instruction_t instr;
    u32 op0_val, op1_val;
    bool cond_action_happened;
    u32 rep_count;
};

// @NOTE: also expects valid data, validate as in simulator.hpp
u32 estimate_instruction_clocks(instruction_metadata_t instr_data);

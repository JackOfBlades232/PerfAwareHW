#pragma once
#include "defs.hpp"
#include "instruction.hpp"

enum proc_type_t {
    e_proc8086,
    e_proc8088
};

struct instruction_metadata_t {
    instruction_t instr;
    u32 op0_val, op1_val;
    bool cond_action_happened;
    u32 rep_count;
    u32 wait_n;
    u32 wide_transfer_cnt, wide_odd_transfer_cnt;
};

// @NOTE: also expects valid data, validate as in simulator.hpp
u32 estimate_instruction_clocks(instruction_metadata_t instr_data, proc_type_t proc_type);

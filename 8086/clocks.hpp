#pragma once
#include "defs.hpp"
#include "instruction.hpp"

// @TODO: this interface?
u32 estimate_instruction_clocks(instruction_t instr, bool cond_action_happened,
                                uint shift_bits);

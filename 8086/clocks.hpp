#pragma once
#include "defs.hpp"
#include "instruction.hpp"

u32 estimate_instruction_clocks(instruction_t instr, bool cond_action_happened);

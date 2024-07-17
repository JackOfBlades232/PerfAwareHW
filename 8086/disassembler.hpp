#pragma once
#include "instruction.hpp"

u32 get_disassembly_ip();

// @NOTE: also expects valid data, validate as in simulator.hpp
u32 output_instrunction_disassembly(instruction_t instr);

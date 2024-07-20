#pragma once

#include "instruction.hpp"
#include "clocks.hpp"

// @NOTE: the validation is not exhaustive. Rather, it checks properties that
//        that if not met break print/execution.

bool validate_instruction(instruction_t instr);
bool validate_instruction_metadata(instruction_metadata_t instr_data);

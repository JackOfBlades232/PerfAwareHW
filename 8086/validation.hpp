#pragma once

#include "instruction.hpp"

// @NOTE: the validation is not exhaustive. Rather, it checks properties that
//        that if not met break print/execution.

bool validate_instruction(instruction_t instr);

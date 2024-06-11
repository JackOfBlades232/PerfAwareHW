#pragma once
#include "instruction.hpp"
#include <cstdio>

namespace output
{

void print(const char *fmt, ...);

void print_reg(reg_access_t reg);
inline void print_word_reg(reg_t reg) { print_reg({reg, 0, 2}); }

void print_intstruction(instruction_t instr);

void set_out_file(FILE *f); 

}

#pragma once
#include "instruction.hpp"
#include <cstdio>

namespace output
{

void print(const char *fmt, ...);
void print_intstruction(instruction_t instr);

void set_out_file(FILE *f);

}

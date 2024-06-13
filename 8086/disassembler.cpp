#include "disassembler.hpp"
#include "print.hpp"

void output_instrunction_disassembly(instruction_t instr, u32 *ip)
{
    *ip += instr.size;

    if (output::instruction_is_printable(instr)) {
        output::print_intstruction(instr);
        output::print("\n");
    }
}

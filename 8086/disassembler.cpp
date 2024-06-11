#include "disassembler.hpp"
#include "print.hpp"

void output_instrunction_disassembly(instruction_t instr)
{
    // @TODO: add output::instruction_is_printable and only do "\n" then
    output::print_intstruction(instr);
    output::print("\n");
}

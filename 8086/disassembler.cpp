#include "disassembler.hpp"
#include "print.hpp"

u32 output_instrunction_disassembly(instruction_t instr)
{
    static u32 ip = 0;
    ip += instr.size;

    if (output::instruction_is_printable(instr)) {
        output::print_intstruction(instr);
        output::print("\n");
    }

    return ip;
}

#include "disassembler.hpp"
#include "print.hpp"

static u32 g_ip = 0;

u32 get_disassembly_ip()
{
    return g_ip;
}

u32 output_instrunction_disassembly(instruction_t instr)
{
    g_ip += instr.size;

    if (output::instruction_is_printable(instr)) {
        output::print_instruction(instr);
        output::print("\n");
    }

    return g_ip;
}

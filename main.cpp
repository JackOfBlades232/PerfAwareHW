#include "defs.hpp"
#include "util.hpp"
#include "instruction.hpp"
#include "encoding.hpp"
#include "decoder.hpp"
#include "memory.hpp"
#include "print.hpp"
#include <cstdio>

/* @TODO:
 * Clean up for sim
 */

enum prog_action_t {
    e_act_simulate,
    e_act_disassemble,
};

template <void (*t_insrunction_processor)(instruction_t)>
bool decode_and_process_instructions(memory_access_t at, u32 bytes)
{
    instruction_table_t table = build_instruction_table();
    decoder_context_t ctx = {};

    while (bytes) {
        instruction_t instr = decode_next_instruction(at, &table, &ctx);
        if (instr.op == e_op_invalid) {
            LOGERR("Failed to decode instruction");
            return false;
        }

        if (bytes >= instr.size) {
            bytes   -= instr.size;
            at.base += instr.size;
            at.size -= instr.size;
        } else {
            LOGERR("Trying to decode outside the instructions mem, the instruction is invalid");
            return false;
        }

        update_decoder_ctx(instr, &ctx);
        t_insrunction_processor(instr);
    }

    return true;
}

int main(int argc, char **argv)
{
    prog_action_t action;
    memory_access_t main_memory = get_main_memory_access();

    const char *fn;
    if (argc < 3) {
        LOGERR("Invalid args, correct format: prog [disasm|sim] [file name] [args]");
        return 1;
    }

    if (streq(argv[1], "disasm"))
        action = e_act_disassemble;
    else if (streq(argv[1], "sim"))
        action = e_act_simulate;
    else {
        LOGERR("Invalid action, must be: disasm|sim");
        return 1;
    }

    fn = argv[2];

    for (int i = 3; i < argc; ++i) {
        if (streq(argv[i], "-o")) {
            ++i;
            if (i >= argc) {
                LOGERR("Specify output file name after -o");
                return 1;
            }
            FILE *out_f = fopen(argv[i], "w");
            if (!out_f) {
                LOGERR("Failed to open output file (%s)", argv[i]);
                return 1;
            }

            output::set_out_file(out_f);
        }
    }

    u32 code_bytes = load_file_to_memory(main_memory, fn);
    if (code_bytes == 0) {
        LOGERR("Failed to load binary (%s)", fn);
        return 1;
    }

    bool res;
    if (action == e_act_disassemble) {
        output::print(";; %s disassembly ;;\n", fn);
        output::print("bits 16\n");
        res = decode_and_process_instructions<output::print_intstruction>(main_memory, code_bytes);
    } else {
        LOGERR("Simulation is not implemented yet");
        res = false;
    }

    return res ? 0 : 1;
}

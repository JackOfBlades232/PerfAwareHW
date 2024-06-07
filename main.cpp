#include "defs.hpp"
#include "util.hpp"
#include "instruction.hpp"
#include "encoding.hpp"
#include "decoder.hpp"
#include "memory.hpp"
#include "print.hpp"

/* @TODO:
 * Clean up for sim
 */

// @HUH: as for a simulation, I maybe should abstract memory acesses with looping and stuff

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

static u8 g_memory[POT(20)];

int main(int argc, char **argv)
{
    memory_access_t main_memory = {g_memory, 0, ARR_CNT(g_memory)};

    // @TODO: allow more files?
    // @TODO: sim by default, or with -s flag, disasm with -d flag
    if (argc != 2) {
        LOGERR("Invalid args, correct format: prog [file name]");
        return 1;
    }

    u32 code_bytes = load_file_to_memory(main_memory, argv[1]);

    // @TEST
    printf(";; %s disassembly ;;\n", argv[1]);
    printf("bits 16\n");
    bool res = decode_and_process_instructions<print_intstruction>(main_memory, code_bytes);
    return res ? 0 : 1;
}

#include "defs.hpp"
#include "consts.hpp"
#include "simulator.hpp"
#include "util.hpp"
#include "instruction.hpp"
#include "encoding.hpp"
#include "decoder.hpp"
#include "memory.hpp"
#include "print.hpp"
#include "disassembler.hpp"
#include "simulator.hpp"
#include "validation.hpp"
#include <cassert>
#include <cstdio>

/* @TODO:
 * Implement validation
 * Add missing instructions (esc) *
 * Resolve TODOs and refac as if for ship
 */

// @TODO: move all this to input module
// @TODO: echo off in interactive mode
bool interactive = false;
void wait_for_input_line()
{
    char c;
    while ((c = getchar()) != EOF) {
        if (c == '\n')
            return;
    }
}

enum prog_action_t {
    e_act_simulate,
    e_act_disassemble,
};

typedef u32 (*instruction_processor_t)(instruction_t);
typedef u32 (*ip_initializer_t)();

template <instruction_processor_t t_process_instr, ip_initializer_t t_init_ip>
inline bool decode_and_process_instructions(memory_access_t at, u32 bytes, bool interactive = false)
{
    instruction_table_t table = build_instruction_table();
    decoder_context_t ctx = {};

    u32 ip = t_init_ip();

    while (bytes - ip > 0 && ip != c_ip_terminate) {
        if (interactive)
            wait_for_input_line();

        instruction_t instr = decode_next_instruction(at, ip, &table, &ctx);
        if (instr.op == e_op_invalid) {
            LOGERR("Failed to decode instruction");
            return false;
        }

        if (!validate_instruction(instr)) {
            LOGERR("Decoded invalid instruction!");
            return false;
        }

        if (ip + instr.size > bytes) {
            LOGERR("Trying to decode outside the instructions mem, the instruction is invalid");
            return false;
        }

        update_decoder_ctx(instr, &ctx);
        ip = t_process_instr(instr);
    }

    return true;
}

int main(int argc, char **argv)
{
    prog_action_t action;
    memory_access_t main_memory = get_main_memory_access();

    const char *fn = nullptr;
    const char *mdump_fn = nullptr;
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

    // @TODO: stop on ret flag
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
        } else if (strncmp(argv[i], "-trace", 6) == 0) {
            if (action != e_act_simulate) {
                LOGERR("Tracing [-trace] only valid for simulation");
                return 1;
            }
            
            const char *p = argv[i] + 6; // skip -trace
            u32 trace_flags = 0;
            if (!*p || streq(p, "=data"))
                trace_flags = e_trace_data_mutation | e_trace_disassembly;
            else if (streq(p, "=clocks"))
                trace_flags = e_trace_disassembly | e_trace_cycles;
            else if (streq(p, "=all"))
                trace_flags = e_trace_data_mutation | e_trace_disassembly | e_trace_cycles;
            else {
                LOGERR("Tracing modes: [-trace(=data|clocks|all)]");
                return 1;
            }

            set_simulation_trace_option(trace_flags, true);
        } else if (streq(argv[i], "-stoponret")) {
            if (action != e_act_simulate) {
                LOGERR("Stop on return [-stoponret] only valid for simulation");
                return 1;
            }

            set_simulation_trace_option(e_trace_stop_on_ret, true);
        } else if (strncmp(argv[i], "-arch=", 6) == 0) {
            if (action != e_act_simulate) {
                LOGERR("Architecture choice [-arch=...] only valid for simulation");
                return 1;
            }

            const char *p = argv[i] + 6; // skip -arch=
            if (streq(p, "8086"))
                ; // default
            else if (streq(p, "8088"))
                set_simulation_trace_option(e_trace_8088cycles, true);
            else {
                LOGERR("Architectures: [-arch=(8086|8088)]");
                return 1;
            }
        } else if (streq(argv[i], "-dump")) {
            ++i;
            if (i >= argc) {
                LOGERR("Specify dump file name after -dump");
                return 1;
            }

            mdump_fn = argv[i];
        } else if (streq(argv[i], "-interactive"))
            interactive = true;
        else {
            LOGERR("Encountered unknown arg [%s]", argv[i]);
            return 1;
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
        res = decode_and_process_instructions<output_instrunction_disassembly, get_disassembly_ip>(
            main_memory, code_bytes, interactive);
    } else {
        output::print("--- %s execution ---\n", fn);
        output_simulation_disclaimer();
        res = decode_and_process_instructions<simulate_instruction_execution, get_simulation_ip>(
            main_memory, code_bytes, interactive);
        output_simulation_results();
    }

    if (mdump_fn) {
        if (!dump_memory_to_file(main_memory, mdump_fn)) {
            LOGERR("Failed to dump memory contents to %s", mdump_fn);
            return 1;
        }
    }

    return res ? 0 : 1;
}

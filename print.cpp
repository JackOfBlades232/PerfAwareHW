#include "print.hpp"
#include "util.hpp"
#include <cassert>
#include <cstdio>
#include <cstdarg>

static FILE *out_f = stdout;

namespace output
{

void print(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(out_f, fmt, args);
    va_end(args);
}

void print_reg(reg_access_t reg)
{
    const char *reg_names[] =
    {
        "al", "ah", "ax",
        "bl", "bh", "bx",
        "cl", "ch", "cx",
        "dl", "dh", "dx",

        nullptr, nullptr, "sp",
        nullptr, nullptr, "bp",
        nullptr, nullptr, "si",
        nullptr, nullptr, "di",

        nullptr, nullptr, "es",
        nullptr, nullptr, "cs",
        nullptr, nullptr, "ss",
        nullptr, nullptr, "ds",
    };

    assert(reg.reg < e_reg_ip);

    // @NOTE: relying on there being three combinations: 01 02 11
    // @FEAT: will not do for 32bit
    int offset = reg.size == 2 ? 2 : (reg.offset == 1 ? 1 : 0);
    print("%s", reg_names[reg.reg*3 + offset]);
}

void print_mem(ea_mem_access_t mem, bool override_seg, reg_t sr)
{
    const char *ea_base_names[e_ea_base_max] =
    {
        "bx+si", "bx+di", "bp+si", "bp+di",
        "si", "di", "bp", "bx",
    };

    const char *sr_names[] = { "es:", "cs:", "ss:", "ds:" };

    assert(mem.base < e_ea_base_max);
    const char *seg_override = override_seg ? sr_names[sr % 0x4] : "";
    if (mem.base == e_ea_base_direct)
        print("%s[%hu]", seg_override, mem.disp);
    else if (mem.disp == 0)
        print("%s[%s]", seg_override, ea_base_names[mem.base]);
    else
        print("%s[%s%+hd]", seg_override, ea_base_names[mem.base], mem.disp);
}

void print_imm(u16 imm, bool is_rel_disp, u32 instr_size)
{
    if (is_rel_disp)
        print("$%+hd+%d", imm, instr_size);
    else
        print("%hd", imm);
}

void print_cs_ip(cs_ip_pair_t cs_ip)
{
    print("%hu:%hu", cs_ip.cs, cs_ip.ip);
}

void print_intstruction(instruction_t instr)
{
    const char *op_mnemonics[e_op_max] =
    {
        "<invalid>",

#define INST(_op, ...) #_op,
#define INSTALT(...)

#include "instruction_table.cpp.inl"
    };

    // We print prefix as part of instruction
    if (instr.op == e_op_lock || instr.op == e_op_rep || instr.op == e_op_segment)
        return;

    if (instr.flags & e_iflags_lock)
        print("lock ");
    if (instr.flags & e_iflags_rep)
        print("rep%s ", (instr.flags & e_iflags_z) ? "" : "nz");

    print("%s", op_mnemonics[instr.op]);

    if (instr.op == e_op_movs ||
        instr.op == e_op_cmps ||
        instr.op == e_op_scas ||
        instr.op == e_op_lods ||
        instr.op == e_op_stos)
    {
        print("%c", (instr.flags & e_iflags_w) ? 'w' : 'b');
    }

    // For locked xchg, nasm wants reg to be second
    if (instr.op == e_op_xchg &&
        (instr.flags & e_iflags_lock) &&
        instr.operands[1].type != e_operand_reg)
    {
        swap(&instr.operands[0], &instr.operands[1]);
    }

    // @TODO: clean up
    if (instr.operand_cnt != 0) {
        print(" ");

        for (int i = 0; i < instr.operand_cnt; ++i) {
            if (i == 1)
                print(", ");

            operand_t *operand = &instr.operands[i];
            switch (operand->type) {
            case e_operand_none: break;

            case e_operand_reg:
                print_reg(operand->data.reg);
                break;
            case e_operand_mem:
                if (instr.flags & e_iflags_far)
                    print("far ");
                // @NOTE: picked up from Casey's code. This produces not-so-neat
                //        prints for something like op [ea], reg, adding a redundant
                //        word/byte, but saves having to store more state in instructions
                else if (instr.operands[0].type != e_operand_reg)
                    print((instr.flags & e_iflags_w) ? "word " : "byte ");
                print_mem(operand->data.mem, instr.flags & e_iflags_seg_override, instr.segment_override);
                break;
            case e_operand_imm:
                print_imm(operand->data.imm, instr.flags & e_iflags_imm_is_rel_disp, instr.size);
                break;
            case e_operand_cs_ip:
                print_cs_ip(operand->data.cs_ip);
                break;
            }
        }
    }

    print("\n");
}

void set_out_file(FILE *f)
{
    assert(f);
    out_f = f;
}

}

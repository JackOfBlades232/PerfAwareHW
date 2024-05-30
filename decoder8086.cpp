#define _CRT_SECURE_NO_WARNINGS
#include <cstdio>
#include <cstdint>
#include <cstddef>
#include <cassert>

static bool g_machine_is_big_endian;

#include "instruction_table.cpp.inc"
#include "decoder_util.cpp.inc"

/* @TODO:
 * Tighten up table and code (pull apart to diff files also, .cpp.inc will be ok)
 * Implement add/sub/cmp with it
 * Implement jumps with it (relative offsets)
 * Check the table again
 * Impl all the other instructions
 */

// @TODO: replace goto failings with try-return where appropriate

const instr_t *decode_instr_type(uint8_t first_byte, FILE *istream, uint8_t *out_second_byte_opt)
{
    // @SPEED: maybe it would be better to constuct some runtime table out
    //         of the hand written one, so that i can just index into it with
    //         byte vals (it would be 2^(8 + 3) = 2048 entries, could also be
    //         pointers).

    bool fetched_second_byte = false;
    for (size_t i = 0; i < ARR_SIZE(instruction_info_table); ++i) {
        const instr_t *info = &instruction_info_table[i];
        bool match = false;

        if ((first_byte & info->desc.first_byte_mask) ==
            info->desc.first_byte_val)
        {
            if (info->desc.requires_second_byte) {
                if (!fetched_second_byte) {
                    TRY_ELSE_RETURN(fetch_mem(istream, out_second_byte_opt), nullptr);
                    fetched_second_byte = true;
                }

                match = (*out_second_byte_opt & info->desc.second_byte_mask) ==
                        info->desc.second_byte_val;
            } else
                match = true;
        }

        if (match)
            return info;
    }

    return nullptr;
}

const char *reg_code_to_name(uint8_t code, bool is_wide)
{
    switch (code) {
    case 0x0:
        return is_wide ? "ax" : "al";
    case 0x1:
        return is_wide ? "cx" : "cl";
    case 0x2:
        return is_wide ? "dx" : "dl";
    case 0x3:
        return is_wide ? "bx" : "bl";
    case 0x4:
        return is_wide ? "sp" : "ah";
    case 0x5:
        return is_wide ? "bp" : "ch";
    case 0x6:
        return is_wide ? "si" : "dh";
    case 0x7:
        return is_wide ? "di" : "bh";
    default:
        return nullptr;
    }
}

const char *lea_code_to_expr(uint8_t code)
{
    switch (code) {
    case 0x0:
        return "bx + si";
    case 0x1:
        return "bx + di";
    case 0x2:
        return "bp + si";
    case 0x3:
        return "bp + di";
    case 0x4:
        return "si";
    case 0x5:
        return "di";
    case 0x6:
        return "bp"; // Not valid for mod == 00 (direct address)
    case 0x7:
        return "bx";
    default:
        return nullptr;
    }
}

bool decode_mem_addr_to_buf(char *buf, size_t bufsize,
                            FILE *istream,
                            uint8_t mod, uint8_t rm_reg)
{
    // @NOTE: spec case: direct addr
    if (mod == 0b00 && rm_reg == 0b110) {
        uint16_t adr;
        TRY_ELSE_RETURN(fetch_mem(istream, &adr), false);
        snprintf(buf, bufsize, "%hu", adr);
        return true;
    }

    switch (mod) {
        // no displacement
        case 0b00:
            snprintf(buf, bufsize, "%s", lea_code_to_expr(rm_reg));
            break;

        // 8bit disp (can be negative, consider as part of 16-bit space)
        case 0b01: {
            int8_t disp;
            TRY_ELSE_RETURN(fetch_mem(istream, &disp), false);
            if (disp > 0)
                snprintf(buf, bufsize, "%s + %hhd", lea_code_to_expr(rm_reg), disp);
            else if (disp < 0)
                snprintf(buf, bufsize, "%s - %hhd", lea_code_to_expr(rm_reg), -disp);
            else
                snprintf(buf, bufsize, "%s", lea_code_to_expr(rm_reg));
        } break;

        // 16bit disp (can be negative)
        case 0b10: {
            int16_t disp;
            TRY_ELSE_RETURN(fetch_mem(istream, &disp), false);
            if (disp > 0)
                snprintf(buf, bufsize, "%s + %hd", lea_code_to_expr(rm_reg), disp);
            else if (disp < 0)
                snprintf(buf, bufsize, "%s - %hd", lea_code_to_expr(rm_reg), -disp);
            else
                snprintf(buf, bufsize, "%s", lea_code_to_expr(rm_reg));
        } break;
    }

    return true;
}

bool decode_rm_to_from_reg_mov(FILE *istream, 
                               uint8_t first_byte, uint8_t second_byte)
{
    bool is_dst  = first_byte & 0b00000010;
    bool is_wide = first_byte & 0b00000001;

    uint8_t mod    = (second_byte & 0b11000000) >> 6;
    uint8_t reg    = (second_byte & 0b00111000) >> 3;
    uint8_t rm_reg = second_byte & 0b00000111;

    if (mod == 0b11) { // reg to reg
        uint8_t src_reg, dst_reg;
        if (is_dst) {
            src_reg = rm_reg;
            dst_reg = reg;
        } else {
            src_reg = reg;
            dst_reg = rm_reg;
        }

        printf("mov %s, %s\n", 
                reg_code_to_name(dst_reg, is_wide), 
                reg_code_to_name(src_reg, is_wide));
    } else { // r/m to/from reg
        char buf[64]; // for expression
        TRY_ELSE_RETURN(
            decode_mem_addr_to_buf(buf, sizeof(buf), istream, mod, rm_reg),
            false);

        if (is_dst)
            printf("mov %s, [%s]\n", reg_code_to_name(reg, is_wide), buf);
        else
            printf("mov [%s], %s\n", buf, reg_code_to_name(reg, is_wide));
    }

    return true;
}

bool decode_imm_to_rm_mov(FILE *istream, 
                          uint8_t first_byte, uint8_t second_byte)
{
    bool is_wide = first_byte & 0b00000001;

    uint8_t mod    = (second_byte & 0b11000000) >> 6;
    uint8_t rm_reg = second_byte & 0b00000111;

    assert((second_byte & 0b00111000) >> 3 == 0b000); // @TODO: ret on fail, user friendly

    char buf[64]; // for expression
    TRY_ELSE_RETURN(decode_mem_addr_to_buf(buf, sizeof(buf), istream, mod, rm_reg), false);

    // @TODO: factor out generic lo-hi fetch
    uint8_t data_low = 0;
    TRY_ELSE_RETURN(fetch_integer_with_endian_swap(istream, &data_low), false);

    if (is_wide) {
        uint8_t data_high = 0;
        TRY_ELSE_RETURN(fetch_integer_with_endian_swap(istream, &data_high), false);
        printf("mov [%s], word %hd\n", buf, 
                ((uint16_t)data_high << 8) | data_low);
    } else
        printf("mov [%s], byte %hhd\n", buf, data_low);

    return true;
}

bool decode_imm_to_reg_mov(FILE *istream, 
                           uint8_t first_byte, uint8_t second_byte)
{
    bool is_wide = first_byte & 0b00001000;
    uint8_t reg  = first_byte & 0b00000111;

    uint8_t data_low = second_byte;
            
    if (is_wide) {
        uint8_t data_high = 0;
        TRY_ELSE_RETURN(fetch_integer_with_endian_swap(istream, &data_high), false);
        printf("mov %s, %hd\n", 
                reg_code_to_name(reg, is_wide),
                ((uint16_t)data_high << 8) | data_low);
    } else {
        printf("mov %s, %hhd\n", 
                reg_code_to_name(reg, is_wide),
                data_low);
    }

    return true;
}

bool decode_acc_to_from_mem_mov(FILE *istream, 
                                uint8_t first_byte, uint8_t second_byte)
{
    bool is_to_mem = first_byte & 0b00000010;
    bool is_wide   = first_byte & 0b00000001;

    uint16_t addr = 0;
    uint8_t addr_low = second_byte;
    if (is_wide) {
        uint8_t addr_high;
        TRY_ELSE_RETURN(fetch_integer_with_endian_swap(istream, &addr_high), false);
        addr = ((uint16_t)addr_high << 8) | addr_low;
    } else
        addr = addr_low;

    if (is_to_mem)
        printf("mov [%hu], %s\n", addr, is_wide ? "ax" : "al");
    else
        printf("mov %s, [%hu]\n", is_wide ? "ax" : "al", addr);

    return true;
}

bool decode_mov(const instr_t *instr_info, FILE *istream, 
                uint8_t first_byte, uint8_t *second_byte_opt)
{
    assert(instr_info);
    assert(!instr_info->desc.requires_second_byte && !second_byte_opt);

    uint8_t second_byte;
    TRY_ELSE_RETURN(fetch_mem(istream, &second_byte), false);

    // @TODO: the weird goto/return dance here is weird. Make proper.
    switch (instr_info->var) {
        case e_iv_rm_to_from_reg:
            return decode_rm_to_from_reg_mov(istream, first_byte, second_byte);
        case e_iv_imm_to_rm:
            return decode_imm_to_rm_mov(istream, first_byte, second_byte);
        case e_iv_imm_to_reg:
            return decode_imm_to_reg_mov(istream, first_byte, second_byte);
        case e_iv_mem_to_acc:
        case e_iv_acc_to_mem:
            return decode_acc_to_from_mem_mov(istream, first_byte, second_byte);

        case e_iv_rm_to_segment:
        case e_iv_segment_to_rm:
            NOT_IMPL();
    
        default:
            return false;
    }

    return true;
}

void check_and_init_endianness()
{
    union {
        uint16_t word;
        uint8_t bytes[2];
    } x;
    x.word = 0x0102;

    g_machine_is_big_endian = x.bytes[0] == (x.word >> 8);
}

#define MAIN_ERROR(_fmt, ...)                 \
    do {                                      \
        fprintf(stderr, _fmt, ##__VA_ARGS__); \
        exit_code = 1;                        \
        goto loop_breakout;                   \
    } while (0)

#define MAIN_TRY_ELSE_ERROR(_try, _fmt, ...) \
    do {                                     \
        if (!(_try))                         \
            MAIN_ERROR(_fmt, ##__VA_ARGS__); \
    } while (0)

int main(int argc, char **argv)
{
    int exit_code = 0;

    check_and_init_endianness();

    assert(argc >= 2);
    FILE *f = fopen(argv[1], "rb");
    assert(f);

    printf(";; %s 16-bit disassembly ;;\n", argv[1]);
    printf("bits 16\n");
    
    uint8_t first_byte = 0;
    while (fetch_integer_with_endian_swap(f, &first_byte)) {
        uint8_t second_byte_opt;
        const instr_t *instr_info = decode_instr_type(first_byte, f, &second_byte_opt);
        if (!instr_info)
            MAIN_ERROR("Invalid instruction code in stream, terminating...\n");

        bool decode_res;
        switch (instr_info->cls) {
        case e_ic_mov:
            decode_res = decode_mov(instr_info, f, first_byte, nullptr);
            break;

        default:
            MAIN_ERROR("Not implemented, terminating...\n");
        }

        if (!decode_res)
            MAIN_ERROR("Instr [%s] decoding failed, terminating...\n", instr_info->desc.text);
    }

loop_breakout:
    fclose(f);
    return exit_code;
}

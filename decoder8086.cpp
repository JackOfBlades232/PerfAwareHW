#define _CRT_SECURE_NO_WARNINGS
#include <cstdio>
#include <cstring>
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

struct xw_t {
    bool x;
    bool w;
};

struct mod_reg_rm_t {
    uint8_t mod;
    uint8_t reg;
    uint8_t rm;
};

bool extract_w(uint8_t byte)
{
    return byte & 0x00000001;
}

xw_t extract_xw(uint8_t byte)
{
    return {
        (bool)(byte & 0b00000010),
        (bool)(byte & 0b00000001)
    };
}

uint8_t extract_secondary_id(uint8_t byte)
{
    return (uint8_t)((byte & 0b00111000) >> 3);
}

mod_reg_rm_t extract_mod_reg_rm(uint8_t byte)
{
    return {
        (uint8_t)(byte >> 6),
        (uint8_t)((byte & 0b00111000) >> 3),
        (uint8_t)(byte & 0b00000111)
    };
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

const char *segment_code_to_name(uint8_t code)
{
    switch (code) {
    case 0x0:
        return "es";
    case 0x1:
        return "cs";
    case 0x2:
        return "ss";
    case 0x3:
        return "ds";
    default:
        return nullptr;
    }
}

const char *arifm_logic_code_to_op_name(uint8_t code)
{
    switch (code) {
    case 0x0:
        return "add";
    case 0x1:
        return "or";
    case 0x2:
        return "adc";
    case 0x3:
        return "sbb";
    case 0x4:
        return "and";
    case 0x5:
        return "sub";
    case 0x6:
        return "xor";
    case 0x7:
        return "cmp";
    default:
        return nullptr;
    }
}

const char *cond_jump_code_to_op_name(uint8_t code)
{
    switch (code) {
    case 0x0:
        return "jo";
    case 0x1:
        return "jno";
    case 0x2:
        return "jb";
    case 0x3:
        return "jae";
    case 0x4:
        return "je";
    case 0x5:
        return "jne";
    case 0x6:
        return "jbe";
    case 0x7:
        return "ja";
    case 0x8:
        return "js";
    case 0x9:
        return "jns";
    case 0xA:
        return "jp";
    case 0xB:
        return "jnp";
    case 0xC:
        return "jl";
    case 0xD:
        return "jge";
    case 0xE:
        return "jle";
    case 0xF:
        return "jg";
    default:
        return nullptr;
    }
}

const char *loop_jump_code_to_op_name(uint8_t code)
{
    switch (code) {
    case 0x0:
        return "loopnz";
    case 0x1:
        return "loopz";
    case 0x2:
        return "loop";
    case 0x3:
        return "jcxz";
    default:
        return nullptr;
    }
}

bool decode_rm_to_buf(char *buf, size_t bufsize,
                      FILE *istream,
                      uint8_t mod, uint8_t rm_reg,
                      bool allow_reg, bool is_wide)
{
    // @NOTE: spec case: reg
    if (mod == 0b11) {
        TRY_ELSE_RETURN(allow_reg, false);
        strncpy(buf, reg_code_to_name(rm_reg, is_wide), bufsize);
    }

    // @NOTE: spec case: direct addr
    if (mod == 0b00 && rm_reg == 0b110) {
        uint16_t adr;
        TRY_ELSE_RETURN(fetch_integer_with_endian_swap(istream, &adr), false);
        snprintf(buf, bufsize, "[%hu]", adr);
        return true;
    }

    // @TODO: check out the [reg + 0] leas in listings. They seem strange.

    switch (mod) {
        // no displacement
        case 0b00:
            snprintf(buf, bufsize, "[%s]", lea_code_to_expr(rm_reg));
            break;

        // 8bit disp (can be negative, consider as part of 16-bit space)
        case 0b01: {
            int8_t disp;
            TRY_ELSE_RETURN(fetch_mem(istream, &disp), false);
            if (disp > 0)
                snprintf(buf, bufsize, "[%s + %hhd]", lea_code_to_expr(rm_reg), disp);
            else if (disp < 0)
                snprintf(buf, bufsize, "[%s - %hhd]", lea_code_to_expr(rm_reg), -disp);
            else
                snprintf(buf, bufsize, "[%s]", lea_code_to_expr(rm_reg));
        } break;

        // 16bit disp (can be negative)
        case 0b10: {
            int16_t disp;
            TRY_ELSE_RETURN(fetch_integer_with_endian_swap(istream, &disp), false);
            if (disp > 0)
                snprintf(buf, bufsize, "[%s + %hd]", lea_code_to_expr(rm_reg), disp);
            else if (disp < 0)
                snprintf(buf, bufsize, "[%s - %hd]", lea_code_to_expr(rm_reg), -disp);
            else
                snprintf(buf, bufsize, "[%s]", lea_code_to_expr(rm_reg));
        } break;
    }

    return true;
}

bool decode_reg_rm_ops_to_buf(char *buf, size_t bufsize,
                              FILE *istream,
                              uint8_t first_byte, uint8_t second_byte,
                              bool is_segment)
{
    auto [is_dst, is_wide]  = extract_xw(first_byte);
    auto [mod, reg, rm_reg] = extract_mod_reg_rm(second_byte);
    if (is_segment) {
        TRY_ELSE_RETURN(!is_wide, false); // seg regs are 16-bit, but instr has 0 in d bit
        is_wide = true;
    }

    const char *reg_nm = is_segment ? segment_code_to_name(reg) : reg_code_to_name(reg, is_wide);
    TRY_ELSE_RETURN(reg_nm, false);

    char buf_rm[64]; // for expression
    TRY_ELSE_RETURN(
        decode_rm_to_buf(buf_rm, sizeof(buf_rm), istream, mod, rm_reg, true, is_wide),
        false);

    if (is_dst)
        snprintf(buf, bufsize, "%s, %s", reg_nm, buf_rm);
    else
        snprintf(buf, bufsize, "%s, %s", buf_rm, reg_nm);

    return true;
}

bool decode_lo_hi_to_buf(char *buf, size_t bufsize, 
                         FILE *istream, bool is_wide, bool is_sign_extending,
                         bool add_word, uint8_t *lo_opt)
{
    if (is_wide && !is_sign_extending) {
        uint16_t data;
        if (!lo_opt)
            TRY_ELSE_RETURN(fetch_integer_with_endian_swap(istream, &data), false);
        else {
            uint8_t hi;
            TRY_ELSE_RETURN(fetch_mem(istream, &hi), false);
            data = (hi << 8) | *lo_opt;
        }
        snprintf(buf, bufsize, "%s%hd", add_word ? "word " : "", data);
    } else {
        uint8_t data;
        if (!lo_opt)
            TRY_ELSE_RETURN(fetch_mem(istream, &data), false);
        else
            data = *lo_opt;

        const char *word = is_wide /* => is_sign_extending */ ? "word " : "byte ";
        snprintf(buf, bufsize, "%s%hhd", add_word ? word : "", data);
    }

    return true;
}

bool decode_rm_to_from_reg_mov(FILE *istream, 
                               uint8_t first_byte, uint8_t second_byte,
                               bool is_segment)
{
    char buf[64];
    TRY_ELSE_RETURN(
        decode_reg_rm_ops_to_buf(buf, sizeof(buf), istream, first_byte, second_byte, is_segment),
        false);

    printf("mov %s\n", buf);
    return true;
}

bool decode_imm_to_rm_mov(FILE *istream, 
                          uint8_t first_byte, uint8_t second_byte)
{
    bool is_wide           = extract_w(first_byte);
    auto [mod, id, rm_reg] = extract_mod_reg_rm(second_byte);
    TRY_ELSE_RETURN(id == 0, false);

    char buf_ea[64], buf_data[32];
    TRY_ELSE_RETURN(
        decode_rm_to_buf(buf_ea, sizeof(buf_ea), istream, mod, rm_reg, false, is_wide), 
        false);
    TRY_ELSE_RETURN(
        decode_lo_hi_to_buf(buf_data, sizeof(buf_data), istream, is_wide, false, true, nullptr), 
        false);

    printf("mov %s, %s\n", buf_ea, buf_data);
    return true;
}

bool decode_imm_to_reg_mov(FILE *istream, 
                           uint8_t first_byte, uint8_t second_byte)
{
    // @TODO: pull out on reoccurance
    bool is_wide = first_byte & 0b00001000;
    uint8_t reg  = first_byte & 0b00000111;

    char buf[32];
    TRY_ELSE_RETURN(
        decode_lo_hi_to_buf(buf, sizeof(buf), istream, is_wide, false, false, &second_byte), 
        false);
            
    printf("mov %s, %s\n", reg_code_to_name(reg, is_wide), buf);
    return true;
}

bool decode_acc_to_from_mem_mov(FILE *istream, 
                                uint8_t first_byte, uint8_t second_byte)
{
    auto [is_to_mem, is_wide] = extract_xw(first_byte);

    char buf[32];
    TRY_ELSE_RETURN(
        decode_lo_hi_to_buf(buf, sizeof(buf), istream, is_wide, false, false, &second_byte), 
        false);

    if (is_to_mem)
        printf("mov [%s], %s\n", buf, is_wide ? "ax" : "al");
    else
        printf("mov %s, [%s]\n", is_wide ? "ax" : "al", buf);

    return true;
}

bool decode_rm_with_reg_arifm_logic(FILE *istream, 
                                    uint8_t first_byte, uint8_t second_byte)
{
    char buf[64];
    TRY_ELSE_RETURN(
        decode_reg_rm_ops_to_buf(buf, sizeof(buf), istream, first_byte, second_byte, false),
        false);

    const char *op = arifm_logic_code_to_op_name(extract_secondary_id(first_byte));
    TRY_ELSE_RETURN(op, false);

    printf("%s %s\n", op, buf);
    return true;
}

bool decode_imm_with_rm_arifm_logic(FILE *istream, 
                                    uint8_t first_byte, uint8_t second_byte)
{
    auto [is_sign_extending, is_wide] = extract_xw(first_byte);
    auto [mod, id, rm_reg]            = extract_mod_reg_rm(second_byte);

    // @TODO: disallow sign ext for ops that don't do it and just have 0 there

    char buf_rm[64];
    TRY_ELSE_RETURN(
        decode_rm_to_buf(buf_rm, sizeof(buf_rm), istream, mod, rm_reg, true, is_wide),
        false);
    char buf_imm[32];
    TRY_ELSE_RETURN(
        decode_lo_hi_to_buf(buf_imm, sizeof(buf_imm), 
                            istream, is_wide, is_sign_extending, mod != 0b11, nullptr),
        false);

    const char *op = arifm_logic_code_to_op_name(id);
    TRY_ELSE_RETURN(op, false);

    printf("%s %s, %s\n", op, buf_rm, buf_imm);
    return true;
}

bool decode_imm_with_acc_arifm_logic(FILE *istream, 
                                     uint8_t first_byte, uint8_t second_byte)
{
    bool is_wide = extract_w(first_byte);

    // @TODO: disallow sign ext for ops that don't do it and just have 0 there

    char buf_imm[32];
    TRY_ELSE_RETURN(
        decode_lo_hi_to_buf(buf_imm, sizeof(buf_imm), istream, is_wide, false, false, &second_byte),
        false);

    const char *op = arifm_logic_code_to_op_name(extract_secondary_id(first_byte));
    TRY_ELSE_RETURN(op, false);

    printf("%s %s, %s\n", op, is_wide ? "ax" : "al", buf_imm);
    return true;
}

void decode_short_jump_offset(char *buf, size_t bufsize, uint8_t data)
{
    data += 2; // jumps are relative to next instr
    if (*(int8_t *)&data > 0)
        snprintf(buf, bufsize, "+%hhd+0", data);
    else if (data == 0)
        snprintf(buf, bufsize, "+0");
    else
        snprintf(buf, bufsize, "%hhd+0", data);
}

void decode_short_jump(const char *op, uint8_t data)
{
    char buf[16];
    decode_short_jump_offset(buf, sizeof(buf), data);
    printf("%s $%s\n", op, buf);
}

bool decode_cond_jump(uint8_t first_byte, uint8_t second_byte)
{
    decode_short_jump(cond_jump_code_to_op_name(first_byte & 0b00001111), second_byte);
    return true;
}

bool decode_loop_jump(uint8_t first_byte, uint8_t second_byte)
{
    decode_short_jump(loop_jump_code_to_op_name(first_byte & 0b00000011), second_byte);
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
    while (fetch_mem(f, &first_byte)) {
        uint8_t second_byte_opt;
        bool decode_res = false;
        
        // @TODO: account for 1-byte instructions (hand crafted unget?)
        MAIN_TRY_ELSE_ERROR(fetch_mem(f, &second_byte_opt), "Failed to fetch second byte, terminating...\n");

        // MOV
        if (check_byte(first_byte, mov_rm_to_from_reg_or_segment_id)) {
            if ((first_byte & 0b00000100) == 0) // rm to/from reg
                decode_res = decode_rm_to_from_reg_mov(f, first_byte, second_byte_opt, false);
            else if ((first_byte & 0b00000101) == 0b100) // 0bXXXXX110 or 0bXXXXX100, rm to/from seg reg
                decode_res = decode_rm_to_from_reg_mov(f, first_byte, second_byte_opt, true);
        } else if (check_byte(first_byte, mov_imm_to_rm_id))
            decode_res = decode_imm_to_rm_mov(f, first_byte, second_byte_opt);
        else if (check_byte(first_byte, mov_imm_to_reg_id))
            decode_res = decode_imm_to_reg_mov(f, first_byte, second_byte_opt);
        else if (check_byte(first_byte, mov_mem_to_from_acc_id))
            decode_res = decode_acc_to_from_mem_mov(f, first_byte, second_byte_opt);
        // Arifm/logic group (ADD/OR/ADC/SBB/AND/SUB/XOR/CMP)
        else if (check_byte(first_byte, al_rm_with_reg_id))
            decode_res = decode_rm_with_reg_arifm_logic(f, first_byte, second_byte_opt);
        else if (check_byte(first_byte, al_imm_with_rm_id))
            decode_res = decode_imm_with_rm_arifm_logic(f, first_byte, second_byte_opt);
        else if (check_byte(first_byte, al_imm_with_acc_id))
            decode_res = decode_imm_with_acc_arifm_logic(f, first_byte, second_byte_opt);
        // Cond jumps/loop jumps
        else if (check_byte(first_byte, cond_j_id))
            decode_res = decode_cond_jump(first_byte, second_byte_opt);
        else if (check_byte(first_byte, loop_j_id))
            decode_res = decode_loop_jump(first_byte, second_byte_opt);
        // Not yet implemented
        else
            MAIN_ERROR("Not implemented, terminating...\n");

        if (!decode_res)
            MAIN_ERROR("Instr [%hhx] decoding failed, terminating...\n", first_byte);
    }

loop_breakout:
    fclose(f);
    return exit_code;
}

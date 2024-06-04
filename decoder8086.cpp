#define _CRT_SECURE_NO_WARNINGS
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <cstddef>
#include <cassert>

static bool g_machine_is_big_endian;

static constexpr uint8_t c_invalid_seg_override = 0xFF;
static uint8_t g_seg_override                   = c_invalid_seg_override;

#include "instruction_table.cpp.inc"
#include "decoder_util.cpp.inc"

/* @TODO:
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

const char *get_lit_prefix(uint8_t mod, bool is_wide)
{
    if (mod == 0b11)
        return "";
    return is_wide ? "word " : "byte ";
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

const char *ff_pack_code_to_op_name(uint8_t code)
{
    switch (code) {
    case 0x0:
        return "inc word";
    case 0x1:
        return "dec word";
    case 0x2:
        return "call";
    case 0x3:
        return "call far";
    case 0x4:
        return "jmp";
    case 0x5:
        return "jmp far";
    case 0x6:
        return "push word";
    default:
        return nullptr;
    }
}

const char *f6_pack_code_to_op_name(uint8_t code)
{
    switch (code) {
    case 0x0:
        return "test";
    case 0x2:
        return "not";
    case 0x3:
        return "neg";
    case 0x4:
        return "mul";
    case 0x5:
        return "imul";
    case 0x6:
        return "div";
    case 0x7:
        return "idiv";
    default:
        return nullptr;
    }
}

const char *bit_instr_code_to_op_name(uint8_t code)
{
    switch (code) {
    case 0x0:
        return "rol";
    case 0x1:
        return "ror";
    case 0x2:
        return "rcl";
    case 0x3:
        return "rcr";
    case 0x4:
        return "shl";
    case 0x5:
        return "shr";
    case 0x7:
        return "sar";
    default:
        return nullptr;
    }
}

const char *string_instr_code_to_op_name(uint8_t code)
{
    switch (code) {
    case 0x2:
        return "movs";
    case 0x3:
        return "cmps";
    case 0x5:
        return "stos";
    case 0x6:
        return "lods";
    case 0x7:
        return "scas";
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

    char buf_seg[4] = "";
    if (g_seg_override != c_invalid_seg_override) {
        snprintf(buf_seg, sizeof(buf_seg), "%s:", segment_code_to_name(g_seg_override));
        g_seg_override = c_invalid_seg_override;
    }

    // @NOTE: spec case: direct addr
    if (mod == 0b00 && rm_reg == 0b110) {
        uint16_t adr;
        TRY_ELSE_RETURN(fetch_integer_with_endian_swap(istream, &adr), false);
        snprintf(buf, bufsize, "%s[%hu]", buf_seg, adr);
        return true;
    }

    switch (mod) {
        // no displacement
        case 0b00:
            snprintf(buf, bufsize, "%s[%s]", buf_seg, lea_code_to_expr(rm_reg));
            break;

        // 8bit disp (can be negative, consider as part of 16-bit space)
        case 0b01: {
            int8_t disp;
            TRY_ELSE_RETURN(fetch_mem(istream, &disp), false);
            if (disp > 0)
                snprintf(buf, bufsize, "%s[%s + %hhd]", buf_seg, lea_code_to_expr(rm_reg), disp);
            else if (disp < 0)
                snprintf(buf, bufsize, "%s[%s - %hhd]", buf_seg, lea_code_to_expr(rm_reg), -disp);
            else
                snprintf(buf, bufsize, "%s[%s]", buf_seg, lea_code_to_expr(rm_reg));
        } break;

        // 16bit disp (can be negative)
        case 0b10: {
            int16_t disp;
            TRY_ELSE_RETURN(fetch_integer_with_endian_swap(istream, &disp), false);
            if (disp > 0)
                snprintf(buf, bufsize, "%s[%s + %hd]", buf_seg, lea_code_to_expr(rm_reg), disp);
            else if (disp < 0)
                snprintf(buf, bufsize, "%s[%s - %hd]", buf_seg, lea_code_to_expr(rm_reg), -disp);
            else
                snprintf(buf, bufsize, "%s[%s]", buf_seg, lea_code_to_expr(rm_reg));
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
                         bool add_word, uint8_t *lo_opt, bool unary_plus = false)
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
        snprintf(buf, bufsize, "%s%s%hd",
                 data >= 0 && unary_plus ? "+" : "",
                 add_word ? "word " : "",
                 data);
    } else {
        uint8_t data;
        if (!lo_opt)
            TRY_ELSE_RETURN(fetch_mem(istream, &data), false);
        else
            data = *lo_opt;

        const char *word = is_wide /* => is_sign_extending */ ? "word " : "byte ";
        snprintf(buf, bufsize, "%s%s%hd",
                 data >= 0 && unary_plus ? "+" : "",
                 add_word ? word : "",
                 data);
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

bool decode_ff_instr(FILE *istream, uint8_t second_byte)
{
    auto [mod, id, rm_reg] = extract_mod_reg_rm(second_byte);

    const char *op = ff_pack_code_to_op_name(id);
    TRY_ELSE_RETURN(op, false);

    char buf[64];
    TRY_ELSE_RETURN(
        decode_rm_to_buf(buf, sizeof(buf),
                         istream,
                         mod, rm_reg,
                         id == 0x2 || id == 0x4 /* in-seg call/jmp */, true),
        false);

    printf("%s %s\n", op, buf);
    return true;
}

bool decode_one_byte_reg_instr(uint8_t byte)
{
    uint8_t op_id  = (byte & 0b00011000) >> 3;
    uint8_t reg_id = byte & 0b00000111;
    const char *reg = reg_code_to_name(reg_id, true);
    const char *op;

    switch (op_id) {
        case 0x0:
            op = "inc";
            break;
        case 0x1:
            op = "dec";
            break;
        case 0x2:
            op = "push";
            break;
        case 0x3:
            op = "pop";
            break;
        default: return false;
    }

    printf("%s %s\n", op, reg);
    return true;
}

bool decode_seg_reg_push_pop(uint8_t byte)
{
    bool is_pop     = byte & 0x1;
    uint8_t seg_id  = (byte & 0b00011000) >> 3;
    const char *seg = segment_code_to_name(seg_id);

    printf("%s %s\n", is_pop ? "pop" : "push", seg);
    return true;
}

bool decode_pop_mem(FILE *istream, uint8_t first_byte, uint8_t second_byte)
{
    auto [mod, id, rm_reg] = extract_mod_reg_rm(second_byte);
    TRY_ELSE_RETURN(id == 0, false);

    char buf[64];
    TRY_ELSE_RETURN(
        decode_rm_to_buf(buf, sizeof(buf), istream, mod, rm_reg, false, true),
        false);
    printf("pop word %s\n", buf);
    return true;
}

bool decode_rmreg_test_xchg(FILE *istream, uint8_t first_byte, uint8_t second_byte)
{
    char buf[64];
    TRY_ELSE_RETURN(
        decode_reg_rm_ops_to_buf(buf, sizeof(buf), istream, 
                                 first_byte, second_byte, false),
        false);
    printf("%s %s\n", (first_byte & 0b10) ? "xchg" : "test", buf); 
    return true;
}

bool decode_reg_acc_xchg(uint8_t byte)
{
    printf("xchg ax, %s\n", reg_code_to_name(byte & 0b111, true));
    return true;
}

bool decode_in_out(uint8_t byte, uint8_t second_byte)
{
    bool is_var_port = byte & 0b00001000;
    bool is_out      = byte & 0b00000010;
    bool is_wide     = extract_w(byte); 

    char buf_port[16] = "dx";
    if (!is_var_port)
        snprintf(buf_port, sizeof(buf_port), "%hhu", second_byte);

    const char *reg = is_wide ? "ax" : "al";

    if (is_out)
        printf("out %s, %s\n", buf_port, reg);
    else
        printf("in %s, %s\n", reg, buf_port);

    return true;
}

bool decode_aam_aad_xlat(uint8_t byte, uint8_t second_byte)
{
    uint8_t id = byte & 0b00000011;
    char buf[16] = "";
    if (id != 0x3 && second_byte != 0x0A)
        snprintf(buf, sizeof(buf), " ,%hhu", second_byte); 
    switch (id) {
    case 0x0:
        printf("aam%s\n", buf);
        break;
    case 0x1:
        printf("aad%s\n", buf);
        break;
    case 0x3:
        printf("xlat\n");
        break;
    default: return false;
    }

    return true;
}

bool decode_ea_op(FILE *istream, uint8_t first_byte, uint8_t second_byte, const char *op)
{
    char buf[64];
    // @NOTE: manually set d and w for convenience
    first_byte |= 0b00000011;
    TRY_ELSE_RETURN(
        decode_reg_rm_ops_to_buf(buf, sizeof(buf), istream, first_byte, second_byte, false),
        false);
    printf("%s %s\n", op, buf);
    return true;
}

bool decode_lea(FILE *istream, uint8_t first_byte, uint8_t second_byte)
{
    return decode_ea_op(istream, first_byte, second_byte, "lea");
}

bool decode_les_lds(FILE *istream, uint8_t first_byte, uint8_t second_byte)
{
    return decode_ea_op(istream, first_byte, second_byte, (first_byte & 0b00000001) ? "lds" : "les");
}

bool decode_flags_instr(uint8_t first_byte)
{
    switch (first_byte & 0b00000011) {
    case 0x0:
        printf("pushf\n");
        break;
    case 0x1:
        printf("popf\n");
        break;
    case 0x2:
        printf("sahf\n");
        break;
    case 0x3:
        printf("lahf\n");
        break;
    }

    return true;
}

// @TODO: pull out byte/word thingy
bool decode_inc_dec_byte(FILE *istream, uint8_t first_byte, uint8_t second_byte)
{
    auto [mod, id, rm_reg] = extract_mod_reg_rm(second_byte);
    TRY_ELSE_RETURN(id == 0 || id == 1, false);
    const char *op = id == 1 ? "dec" : "inc";

    char buf[64];
    TRY_ELSE_RETURN(
        decode_rm_to_buf(buf, sizeof(buf), istream, mod, rm_reg, true, false),
        false);

    bool is_reg = mod == 0b11;
    printf("%s %s%s\n", op, is_reg ? "" : "byte ", buf);
    return true;
}

bool decode_aa_seg_prefix_instr(uint8_t first_byte)
{
    bool is_segment_prefix = !extract_w(first_byte);
    uint8_t id = (first_byte & 0b00011000) >> 3;
    const char *name;
    switch (id) {
    case 0x0:
        name = is_segment_prefix ? "es" : "daa";
        break;
    case 0x1:
        name = is_segment_prefix ? "cs" : "das";
        break;
    case 0x2:
        name = is_segment_prefix ? "ss" : "aaa";
        break;
    case 0x3:
        name = is_segment_prefix ? "ds" : "aas";
        break;
    }

    if (is_segment_prefix)
        g_seg_override = id;
    else
        printf("%s\n", name);

    return true;
}

bool decode_f6_pack_instr(FILE *istream, uint8_t first_byte, uint8_t second_byte)
{
    bool is_wide           = extract_w(first_byte);
    auto [mod, id, rm_reg] = extract_mod_reg_rm(second_byte);

    const char *op = f6_pack_code_to_op_name(id);
    char buf_rm[64];
    TRY_ELSE_RETURN(
        decode_rm_to_buf(buf_rm, sizeof(buf_rm), istream, mod, rm_reg, true, is_wide),
        false);

    if (id == 0) { // test
        char buf_imm[64];
        TRY_ELSE_RETURN(
            decode_lo_hi_to_buf(buf_imm, sizeof(buf_imm), istream, is_wide, false, true, nullptr),
            false);

        printf("test %s, %s\n", buf_rm, buf_imm);
    } else
        printf("%s %s%s\n", op, get_lit_prefix(mod, is_wide), buf_rm);

    return true;
}

bool decode_cbw_cwd(uint8_t byte)
{
    printf("%s\n", extract_w(byte) ? "cwd" : "cbw");
    return true;
}

bool decode_bit_instr(FILE *istream, uint8_t first_byte, uint8_t second_byte)
{
    auto [is_cl, is_wide]  = extract_xw(first_byte);
    auto [mod, id, rm_reg] = extract_mod_reg_rm(second_byte);

    const char *op = bit_instr_code_to_op_name(id);
    char buf_rm[64];
    TRY_ELSE_RETURN(
        decode_rm_to_buf(buf_rm, sizeof(buf_rm), istream, mod, rm_reg, true, is_wide),
        false);

    printf("%s %s%s, %s\n", op, get_lit_prefix(mod, is_wide), buf_rm, is_cl ? "cl" : "1");
    return true;
}

bool decode_test_acc(FILE *istream, uint8_t first_byte, uint8_t second_byte)
{
    bool is_wide = extract_w(first_byte);
    const char *acc = is_wide ? "ax" : "al";

    char buf[32];
    TRY_ELSE_RETURN(
        decode_lo_hi_to_buf(buf, sizeof(buf), istream, is_wide, false, false, &second_byte),
        false);

    printf("test %s, %s\n", acc, buf);
    return true;
}

bool decode_string_instr(uint8_t byte, const char *prefix = nullptr)
{
    TRY_ELSE_RETURN(byte >> 4 == 0b1010, false);
    char addition = extract_w(byte) ? 'w' : 'b';
    uint8_t id = (byte >> 1) & 0b111;
    const char *op = string_instr_code_to_op_name(id);
    TRY_ELSE_RETURN(op, false);
    
    printf("%s%s%s%c\n", prefix ? prefix : "", prefix ? " " : "", op, addition);
    return true;
}

bool decode_ret_instr(FILE *istream, uint8_t first_byte, uint8_t second_byte)
{
    bool has_disp  = !extract_w(first_byte);
    const char *op = (first_byte & 0b1000) ? "retf" : "ret";
    // @TODO: check if ret for intersegment has different mnemonic
    if (!has_disp)
        printf("%s\n", op);
    else {
        char buf[32];
        TRY_ELSE_RETURN(
            decode_lo_hi_to_buf(buf, sizeof(buf), istream, true, false, false, &second_byte), 
            false);

        printf("%s %s\n", op, buf);
    }

    return true;
}

bool decode_interrupt_instr(uint8_t first_byte, uint8_t second_byte)
{
    uint8_t type = first_byte & 0b11;
    switch (type) {
    case 0x0:
        printf("int3\n");
        break;
    case 0x1:
        printf("int %hhu\n", second_byte);
        break;
    case 0x2:
        printf("into\n");
        break;
    case 0x3:
        printf("iret\n");
        break;
    }

    return true;
}

bool decode_process_ctrl_instr(uint8_t byte)
{
    uint8_t type = byte & 0b111;
    switch (type) {
    case 0x0:
        printf("clc\n");
        break;
    case 0x1:
        printf("stc\n");
        break;
    case 0x2:
        printf("cli\n");
        break;
    case 0x3:
        printf("sti\n");
        break;
    case 0x4:
        printf("cld\n");
        break;
    case 0x5:
        printf("std\n");
        break;
    default: return false;
    }

    return true;
}

bool decode_call_jmp_direct(FILE *istream, uint8_t first_byte, uint8_t second_byte)
{
    uint8_t type = first_byte == 0b10011010 ? 0xFF : (first_byte & 0b11);
    const char *op = (type == 0x0 || type == 0xFF) ? "call" : "jmp";

    char buf_where[32];
    TRY_ELSE_RETURN(
        decode_lo_hi_to_buf(buf_where, sizeof(buf_where), istream,
                            type != 0b11, false, false, &second_byte, true),
        false);

    if (type == 0b10 || type == 0xFF) {
        char buf_pref[32];
        TRY_ELSE_RETURN(
            decode_lo_hi_to_buf(buf_pref, sizeof(buf_pref), istream,
                                true, false, false, nullptr),
            false);

        printf("%s %s:%s\n", op, buf_pref, buf_where);
    } else
        printf("%s $%s+3\n", op, buf_where);

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
    bool carry_over = false;
    while (carry_over || fetch_mem(f, &first_byte)) {
        carry_over = false;

        uint8_t second_byte;
        bool decode_res = false;
        
        // @TODO: save instruction info instead of printing, so that errors are
        //        dealt with properly, and for possible simulation
        bool fetched_second_byte = fetch_mem(f, &second_byte);

        // MOV
        if (check_byte(first_byte, mov_rm_reg_lea_pop_pack_id)) {
            if ((first_byte & 0b00000100) == 0) // rm to/from reg
                decode_res = decode_rm_to_from_reg_mov(f, first_byte, second_byte, false);
            else if ((first_byte & 0x1) == 0x0) // 0bXXXXX110 or 0bXXXXX100, rm to/from seg reg
                decode_res = decode_rm_to_from_reg_mov(f, first_byte, second_byte, true);
            else if ((first_byte & 0b11) == 0b01)
                decode_res = decode_lea(f, first_byte, second_byte);
            else
                decode_res = decode_pop_mem(f, first_byte, second_byte);
        } else if (check_byte(first_byte, mov_imm_to_rm_id))
            decode_res = decode_imm_to_rm_mov(f, first_byte, second_byte);
        else if (check_byte(first_byte, mov_imm_to_reg_id))
            decode_res = decode_imm_to_reg_mov(f, first_byte, second_byte);
        else if (check_byte(first_byte, mov_mem_to_from_acc_id))
            decode_res = decode_acc_to_from_mem_mov(f, first_byte, second_byte);
        // Arifm/logic group (ADD/OR/ADC/SBB/AND/SUB/XOR/CMP)
        else if (check_byte(first_byte, al_rm_with_reg_id))
            decode_res = decode_rm_with_reg_arifm_logic(f, first_byte, second_byte);
        else if (check_byte(first_byte, al_imm_with_rm_id))
            decode_res = decode_imm_with_rm_arifm_logic(f, first_byte, second_byte);
        else if (check_byte(first_byte, al_imm_with_acc_id))
            decode_res = decode_imm_with_acc_arifm_logic(f, first_byte, second_byte);
        // Cond jumps/loop jumps
        else if (check_byte(first_byte, cond_j_id))
            decode_res = decode_cond_jump(first_byte, second_byte);
        else if (check_byte(first_byte, loop_j_id))
            decode_res = decode_loop_jump(first_byte, second_byte);
        // 0xFF instr pack (INC16/DEC16/CALLJMP 16 indirect inter/intra /PUSH 16)
        else if (check_byte(first_byte, ff_pack_id)) 
            decode_res = decode_ff_instr(f, second_byte);
        // Register INC/DEC/PUSH/POP
        else if (check_byte(first_byte, reg_one_byte_pack_id)) {
            decode_res = decode_one_byte_reg_instr(first_byte);
            carry_over = true;
        } else if (check_byte(first_byte, seg_reg_push_pop_id)) {
            decode_res = decode_seg_reg_push_pop(first_byte);
            carry_over = true;
        }
        // Reg/rm xchg/test
        else if (check_byte(first_byte, rmreg_test_xchg_id))
            decode_res = decode_rmreg_test_xchg(f, first_byte, second_byte);
        else if (check_byte(first_byte, reg_acc_xchg_id)) {
            decode_res = decode_reg_acc_xchg(first_byte);
            carry_over = true;
        }
        // in/out
        else if (check_byte(first_byte, in_out_id)) {
            decode_res = decode_in_out(first_byte, second_byte);
            carry_over = first_byte & 0b00001000; // if variable port
        }
        // aam/aad/xlat
        else if (check_byte(first_byte, aam_aad_xlat_id)) {
            decode_res = decode_aam_aad_xlat(first_byte, second_byte);
            carry_over = (first_byte & 0b11) == 0b11; // only xlat
        }
        // les/lds
        else if (check_byte(first_byte, les_lds_id))
            decode_res = decode_les_lds(f, first_byte, second_byte);
        // Flags instructions (LAHF, SAHF, PUSHF, POPF)
        else if (check_byte(first_byte, flags_instr_id)) {
            decode_res = decode_flags_instr(first_byte);
            carry_over = true;
        }
        // 8-byte INC/DEC
        else if (check_byte(first_byte, inc_dec_byte_id))
            decode_res = decode_inc_dec_byte(f, first_byte, second_byte);
        // DAA/DAS/AAA/AAS + prefices ES:/CS:/SS:/DS:
        else if (check_byte(first_byte, aa_seg_prefix_pack_id)) {
            decode_res = decode_aa_seg_prefix_instr(first_byte);
            carry_over = true;
        }
        // TEST imm rm/NOT/NEG/MUL/IMUL/DIV/IDIV pack
        else if (check_byte(first_byte, f6_pack_id))
            decode_res = decode_f6_pack_instr(f, first_byte, second_byte);
        // CBW/CWD
        else if (check_byte(first_byte, cbw_cwd_id)) {
            decode_res = decode_cbw_cwd(first_byte);
            carry_over = true;
        }
        // Logic pack (SHL+SAL/SHR/SAR/ROL/ROR/RCL/RCR)
        else if (check_byte(first_byte, bit_pack_id))
            decode_res = decode_bit_instr(f, first_byte, second_byte);
        // TEST with accum
        else if (check_byte(first_byte, test_acc_id))
            decode_res = decode_test_acc(f, first_byte, second_byte);
        // REP
        else if (check_byte(first_byte, rep_id))
            decode_res = decode_string_instr(second_byte, extract_w(first_byte) ? "rep" : "repnz");
        else if (check_byte(first_byte, movs_cmps_id) ||
                 (check_byte(first_byte, stos_lods_scas_id) && (first_byte & 0b110) != 0b000))
        {
            decode_res = decode_string_instr(first_byte);
            carry_over = true;
        }
        // RET
        else if (check_byte(first_byte, ret_id)) {
            decode_res = decode_ret_instr(f, first_byte, second_byte);
            carry_over = first_byte & 0b1;
        }
        // INT/IRET/INTO
        else if (check_byte(first_byte, interrupt_id)) {
            decode_res = decode_interrupt_instr(first_byte, second_byte);
            carry_over = (first_byte & 0b11) != 0b01; // not int with param
        }
        // CLC/STC/CLI/STI/CLD/STD
        else if (check_byte(first_byte, process_ctrl_pack_id) &&
                 (first_byte & 0b110) != 0b110)
        {
            decode_res = decode_process_ctrl_instr(first_byte);
            carry_over = true;
        }
        // HLT/CMC
        else if (check_byte(first_byte, hlt_cmc_id)) {
            printf("%s\n", extract_w(first_byte) ? "cmc" : "hlt");
            decode_res = carry_over = true;
        }
        // WAIT, LOCK (@TODO: validation for lock like for rep?)
        else if (first_byte == 0b10011011) {
            printf("wait\n");
            decode_res = carry_over = true;
        } else if (first_byte == 0b11110000) {
            printf("lock ");
            decode_res = carry_over = true;
        }
        // CALL/JMP direct (w/out call to far label, factor it in manually
        else if (check_byte(first_byte, call_jmp_direct_id) ||
                 first_byte == 0b10011010)
        {
            decode_res = decode_call_jmp_direct(f, first_byte, second_byte);
        }
        // Not yet implemented
        else
            MAIN_ERROR("Invalid instruction byte [%hhx], terminating...\n", first_byte);

        if (!fetched_second_byte) {
            if (!carry_over) {
                MAIN_ERROR("Instr [%hhx] requires second byte, but encountered EOF, terminating...\n",
                           first_byte);
            } else
                break;
        }

        if (!decode_res)
            MAIN_ERROR("Instr [%hhx] decoding failed, terminating...\n", first_byte);

        if (carry_over)
            first_byte = second_byte;
    }

loop_breakout:
    fclose(f);
    return exit_code;
}

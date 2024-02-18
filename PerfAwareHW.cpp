#include <cstdio>
#include <cstdint>
#include <cassert>

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
        return 0;
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
        return 0;
    }
}

inline uint16_t u16_swap_bytes(uint16_t n)
{
    return (n >> 8) | ((n & 0xFF) << 8);
}

// _stream : FILE *; _out: uint8_t * or uint16_t *; _break_label: label at the end of loop
// Reverse byte order since fread reads second bit as low (@TODO: endianness?)
#define FETCH_OR_BREAKOUT_WREORDER(_stream, _out, _break_label) \
    do { \
        if (fread(_out, sizeof(*_out), 1, _stream) != 1) goto _break_label; \
        if (sizeof(*_out) > 1) \
            *_out = u16_swap_bytes(*_out); \
    } while (0)

#define FETCH_OR_BREAKOUT_NOREORDER(_stream, _out, _break_label) \
    do { if (fread(_out, sizeof(*_out), 1, _stream) != 1) goto _break_label; } while (0)

// @NOTE: presume that the expr fits in the buf 
// (if it doesn't, it is still safe, but does not indicate failure)
bool get_mem_addr_str(char *buf, size_t bufsize, uint8_t rm_reg, uint8_t mod, FILE *istream)
{
    // @NOTE: spec case: direct addr
    if (mod == 0x0 /* 00 */ && rm_reg == 0x6 /* 110 */) {
        uint16_t adr;
        FETCH_OR_BREAKOUT_NOREORDER(istream, &adr, fail);
        snprintf(buf, bufsize, "%hu", adr);
        return true;
    }

    switch (mod) {
        // no displacement
        case 0x0: /* 00 */
            snprintf(buf, bufsize, "%s", lea_code_to_expr(rm_reg));
            break;

            // 8bit disp (can be negative, consider as part of 16-bit space)
        case 0x1: /* 01 */
            {
                int8_t disp;
                FETCH_OR_BREAKOUT_NOREORDER(istream, &disp, fail);
                if (disp > 0)
                    snprintf(buf, bufsize, "%s + %hhd", lea_code_to_expr(rm_reg), disp);
                else if (disp < 0)
                    snprintf(buf, bufsize, "%s - %hhd", lea_code_to_expr(rm_reg), -disp);
                else
                    snprintf(buf, bufsize, "%s", lea_code_to_expr(rm_reg));
            } break;

            // 16bit disp (can be negative)
        case 0x2: /* 11 */
            {
                int16_t disp;
                FETCH_OR_BREAKOUT_NOREORDER(istream, &disp, fail);
                if (disp > 0)
                    snprintf(buf, bufsize, "%s + %hd", lea_code_to_expr(rm_reg), disp);
                else if (disp < 0)
                    snprintf(buf, bufsize, "%s - %hd", lea_code_to_expr(rm_reg), -disp);
                else
                    snprintf(buf, bufsize, "%s", lea_code_to_expr(rm_reg));
            } break;
    }

    return true;

fail:
    return false;
}

int main(int argc, char **argv)
{
    assert(argc >= 2);
    FILE *f;
    assert(f = fopen(argv[1], "rb"));

    printf(";; %s 16-bit disassembly ;;\n", argv[1]);
    printf("bits 16\n");
    
    // @TODO: errors
    // @TODO: there are on-byte instructions. Fetch only one byte on iteration init
    for (;;) {
        uint16_t instr = 0;
        FETCH_OR_BREAKOUT_WREORDER(f, &instr, loop_breakout);

        uint8_t first_byte = instr >> 8;
        uint8_t second_byte = instr & 0xFF;

        const uint8_t mov_reg_to_rm_prefix = 0x88;
        const uint8_t mov_imm_to_reg_prefix = 0xB0;
        const uint8_t mov_imm_to_rm_prefix = 0xC6;
        const uint8_t mov_accum_to_mem_prefix = 0xA0;

        // reg to rm/reg
        if ((first_byte & 0xFC) == mov_reg_to_rm_prefix) {
            bool is_dst = first_byte & 0x02;
            bool is_wide = first_byte & 0x01;

            uint8_t mod = (second_byte & 0xC0 /* 11000000 */) >> 6;
            uint8_t reg = (second_byte & 0x38 /* 00111000 */) >> 3;
            uint8_t rm_reg = second_byte & 0x7 /* 00000111 */;

            if (mod == 0x3 /* 11 */) { // reg to reg
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
                if (!get_mem_addr_str(buf, sizeof(buf), rm_reg, mod, f))
                    goto loop_breakout;

                if (is_dst)
                    printf("mov %s, [%s]\n", reg_code_to_name(reg, is_wide), buf);
                else
                    printf("mov [%s], %s\n", buf, reg_code_to_name(reg, is_wide));
            }
        // Imm to reg
        } else if ((first_byte & 0xF0) == mov_imm_to_reg_prefix) {
            bool is_wide = first_byte & 0x8;
            uint8_t reg = first_byte & 0x7;
            uint8_t data_low = second_byte;
            
            if (is_wide) {
                uint8_t data_high = 0;
                FETCH_OR_BREAKOUT_WREORDER(f, &data_high, loop_breakout);
                printf("mov %s, %hd\n", 
                        reg_code_to_name(reg, is_wide),
                        ((uint16_t)data_high << 8) | data_low);
            } else {
                printf("mov %s, %hhd\n", 
                        reg_code_to_name(reg, is_wide),
                        data_low);
            }
        // Imm to r/m
        } else if ((first_byte & 0xFE) == mov_imm_to_rm_prefix) {
            bool is_wide = first_byte & 0x01;
            uint8_t mod = (second_byte & 0xC0 /* 11000000 */) >> 6;
            assert((second_byte & 0x38 /* 00111000 */) == 0); // reg field = 0
            uint8_t rm_reg = second_byte & 0x7 /* 00000111 */;

            char buf[64]; // for expression
            if (!get_mem_addr_str(buf, sizeof(buf), rm_reg, mod, f))
                goto loop_breakout;

            // @TODO: factor out with imm to reg
            uint8_t data_low = 0;
            FETCH_OR_BREAKOUT_WREORDER(f, &data_low, loop_breakout);

            if (is_wide) {
                uint8_t data_high = 0;
                FETCH_OR_BREAKOUT_WREORDER(f, &data_high, loop_breakout);
                printf("mov [%s], word %hd\n", buf, 
                        ((uint16_t)data_high << 8) | data_low);
            } else
                printf("mov [%s], byte %hhd\n", buf, data_low);
        // Accum to/from mem
        } else if ((first_byte & 0xFC) == mov_accum_to_mem_prefix) {
            bool is_to_mem = first_byte & 0x2;
            bool is_wide = first_byte & 0x1;

            uint16_t addr = 0;
            uint8_t addr_low = second_byte;
            if (is_wide) {
                uint8_t addr_high;
                FETCH_OR_BREAKOUT_WREORDER(f, &addr_high, loop_breakout);
                addr = ((uint16_t)addr_high << 8) | addr_low;
            } else
                addr = addr_low;

            if (is_to_mem)
                printf("mov [%hu], %s\n", addr, is_wide ? "ax" : "al");
            else
                printf("mov %s, [%hu]\n", is_wide ? "ax" : "al", addr);
        }
    }

loop_breakout:
    fclose(f);
    return 0;
}

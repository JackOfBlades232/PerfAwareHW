#include <cstdio>
#include <cstdint>
#include <cassert>

const uint8_t mov_prefix = 0x88;
const uint8_t mov_reg_to_reg_mod = 0xC0;

const char *reg_code_to_name(uint8_t code, bool is_wide)
{
    switch (code) {
    case 0x0:
        return is_wide ? "ax" : "al";
    case 0x1:
        return is_wide ? "bx" : "bl";
    case 0x2:
        return is_wide ? "cx" : "cl";
    case 0x3:
        return is_wide ? "dx" : "dl";
    case 0x4:
        return is_wide ? "sp" : "ah";
    case 0x5:
        return is_wide ? "bp" : "bh";
    case 0x6:
        return is_wide ? "si" : "ch";
    case 0x7:
        return is_wide ? "di" : "dh";
    default:
        return 0;
    }
}

int main(int argc, char **argv)
{
    assert(argc >= 2);
    FILE *f;
    assert(f = fopen(argv[1], "rb"));

    printf(";; %s 16-bit disassembly ;;\n", argv[1]);
    printf("bits 16\n");
    
    uint16_t instr = 0;
    size_t bytes_read = 0;
    while ((bytes_read = fread(&instr, sizeof(instr), 1, f)) == 1) {
        uint8_t first_byte = instr >> 8;
        uint8_t second_byte = instr & 0xFF;
        switch (first_byte & 0xFC) {
        case mov_prefix:
        {
            bool is_dst = first_byte & 0x02;
            bool is_wide = first_byte & 0x01;
            uint8_t mod = second_byte & 0xFC;
            if (mod == mov_reg_to_reg_mod) {
                uint8_t reg = (second_byte ^ mod) >> 3;
                uint8_t rm_reg = second_byte & 0x7;

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
            } else {

            }
        } break;

        default:
            break;
        }
    }

    fclose(f);
    return 0;
}

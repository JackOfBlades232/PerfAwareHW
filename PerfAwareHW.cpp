#define _CRT_SECURE_NO_WARNINGS
#include <cstdio>
#include <cstdint>
#include <cstddef>
#include <cassert>

/* @TODO:
 * Tighten up table and code (pull apart to diff files also, .cpp.inc will be ok)
 * Implement add/sub/cmp with it
 * Implement jumps with it (relative offsets)
 * Check the table again
 * Impl all the other instructions
 */

// @TODO: replace goto failings with try-return where appropriate

enum instr_class_t {
    e_ic_mov = 0,

    e_ic_push,
    e_ic_pop,

    e_ic_xchg,

    e_ic_in,
    e_ic_out,

    e_ic_xlat,
    e_ic_lea,
    e_ic_lds,
    e_ic_les,
    e_ic_lahf,
    e_ic_sahf,
    e_ic_pushf,
    e_ic_popf,

    e_ic_add,
    e_ic_adc,
    e_ic_inc,

    e_ic_aaa,
    e_ic_daa,

    e_ic_sub,
    e_ic_sbb,

    e_ic_dec,
    e_ic_neg,
    e_ic_cmp,

    e_ic_aas,
    e_ic_das,
    e_ic_mul,
    e_ic_imul,
    e_ic_aam,
    e_ic_div,
    e_ic_idiv,
    e_ic_aad,
    e_ic_cbw,
    e_ic_cwd,

    e_ic_not,
    e_ic_shl, // Identical to sal
    e_ic_shr,
    e_ic_sar,
    e_ic_rol,
    e_ic_ror,
    e_ic_rcl,
    e_ic_rcr,

    e_ic_and,
    e_ic_test,
    e_ic_or,
    e_ic_xor,

    e_ic_rep,
    e_ic_movs,
    e_ic_cmps,
    e_ic_scas,
    e_ic_lods,
    e_ic_stos,

    e_ic_call,
    e_ic_jmp,
    e_ic_ret,

    e_ic_je,  // or jz
    e_ic_jl,  // or jnge
    e_ic_jle, // or jng
    e_ic_jb,  // or jnae
    e_ic_jbe, // or jna
    e_ic_jp,  // or jpe
    e_ic_jo,
    e_ic_js,
    e_ic_jne, // or jnz
    e_ic_jge, // or jnl
    e_ic_jg,  // or jnle
    e_ic_jae, // or jnb
    e_ic_ja,  // or jnbe
    e_ic_jnp, // or jpo
    e_ic_jno,
    e_ic_jns,

    e_ic_loop,
    e_ic_loope,  // or loopnz
    e_ic_loopne, // or loopne
    e_ic_jcxz,

    e_ic_int,
    e_ic_into,
    e_ic_iret,

    e_ic_clc,
    e_ic_cmc,
    e_ic_stc,
    e_ic_cld,
    e_ic_std,
    e_ic_cli,
    e_ic_sti,
    e_ic_hlt,
    e_ic_wait,
    e_ic_esc,
    e_ic_lock,
    e_ic_segment,

    e_ic_max
};

enum instr_variation_t {
    e_iv_none = 0,

    // @HUH: is reuse of these variations to ambiguous?
    // mov/arifm
    e_iv_rm_to_from_reg,
    e_iv_imm_to_rm,
    e_iv_imm_to_reg,
    e_iv_mem_to_acc,
    e_iv_acc_to_mem,
    e_iv_rm_to_segment,
    e_iv_segment_to_rm,

    e_iv_rm_with_reg,
    e_iv_reg_with_acc,
    e_iv_imm_to_acc,

    // push/pop/general
    e_iv_rm,
    e_iv_reg,
    e_iv_segment_reg,

    // in/out
    e_iv_fixed_port,
    e_iv_variable_port,

    // Jumps (call/jmp/ret)
    e_iv_direct_within_segment,
    e_iv_direct_within_segment_short,
    e_iv_indirect_within_segment,
    e_iv_direct_intersegment,
    e_iv_indirect_intersegment,

    e_iv_within_segment,
    e_iv_within_segment_add_imm_to_sp,
    e_iv_intersegment,
    e_iv_intersegment_add_imm_to_sp,

    // Interrupts
    e_iv_int_type_specified,
    e_iv_int_type_unspecified,

    e_iv_max
};

struct intstr_desc_t {
    const char *text;
    bool requires_second_byte;
    uint8_t first_byte_val;
    uint8_t first_byte_mask;
    uint8_t second_byte_val;
    uint8_t second_byte_mask;
};

struct instr_t {
    instr_class_t cls;
    instr_variation_t var;
    intstr_desc_t desc;
};

static const instr_t instruction_info_table[] =
{
    {
        .cls  = e_ic_mov,
        .var  = e_iv_rm_to_from_reg,
        .desc = {
            .text                 = "MOV Register/memory to/from register",
            .requires_second_byte = false,
            .first_byte_val       = 0b10001000,
            .first_byte_mask      = 0b11111100
        }
    },
    {
        .cls  = e_ic_mov,
        .var  = e_iv_imm_to_rm,
        .desc = {
            .text                 = "MOV Immediate to register/memory",
            .requires_second_byte = false, // All other second-byte codes are unused
            .first_byte_val       = 0b11000110,
            .first_byte_mask      = 0b11111110
        }
    },
    {
        .cls  = e_ic_mov,
        .var  = e_iv_imm_to_reg,
        .desc = {
            .text                 = "MOV Immediate to register",
            .requires_second_byte = false,
            .first_byte_val       = 0b10110000,
            .first_byte_mask      = 0b11110000
        }
    },
    {
        .cls  = e_ic_mov,
        .var  = e_iv_mem_to_acc,
        .desc = {
            .text                 = "MOV Memory to accumulator",
            .requires_second_byte = false,
            .first_byte_val       = 0b10100000,
            .first_byte_mask      = 0b11111110
        }
    },
    {
        .cls  = e_ic_mov,
        .var  = e_iv_acc_to_mem,
        .desc = {
            .text                 = "MOV Accumulator to memory",
            .requires_second_byte = false,
            .first_byte_val       = 0b10100010,
            .first_byte_mask      = 0b11111110
        }
    },
    {
        .cls  = e_ic_mov,
        .var  = e_iv_rm_to_segment,
        .desc = {
            .text                 = "MOV Register/memory to segment register",
            .requires_second_byte = false, // All other second-byte codes are unused
            .first_byte_val       = 0b10001110,
            .first_byte_mask      = 0b11111111
        }
    },
    {
        .cls  = e_ic_mov,
        .var  = e_iv_segment_to_rm,
        .desc = {
            .text                 = "MOV Segment register to register/memory",
            .requires_second_byte = false, // All other second-byte codes are unused
            .first_byte_val       = 0b10001100,
            .first_byte_mask      = 0b11111111
        }
    },

    {
        .cls  = e_ic_push,
        .var  = e_iv_rm,
        .desc = {
            .text                 = "PUSH Register/memory",
            .requires_second_byte = true,
            .first_byte_val       = 0b11111111,
            .first_byte_mask      = 0b11111111,
            .second_byte_val      = 0b00110000,
            .second_byte_mask     = 0b00111000
        }
    },
    {
        .cls  = e_ic_push,
        .var  = e_iv_reg,
        .desc = {
            .text                 = "PUSH Register",
            .requires_second_byte = false,
            .first_byte_val       = 0b01010000,
            .first_byte_mask      = 0b11111000
        }
    },
    {
        .cls  = e_ic_push,
        .var  = e_iv_segment_reg,
        .desc = {
            .text                 = "PUSH Segment register",
            .requires_second_byte = false,
            .first_byte_val       = 0b00000110,
            .first_byte_mask      = 0b11100111
        }
    },
    {
        .cls  = e_ic_pop,
        .var  = e_iv_rm,
        .desc = {
            .text                 = "POP Register/memory",
            .requires_second_byte = false, // All other second-byte codes are unused
            .first_byte_val       = 0b10001111,
            .first_byte_mask      = 0b11111111
        }
    },
    {
        .cls  = e_ic_pop,
        .var  = e_iv_reg,
        .desc = {
            .text                 = "POP Register",
            .requires_second_byte = false,
            .first_byte_val       = 0b01011000,
            .first_byte_mask      = 0b11111000
        }
    },
    {
        .cls  = e_ic_pop,
        .var  = e_iv_segment_reg,
        .desc = {
            .text                 = "POP Segment register",
            .requires_second_byte = false,
            .first_byte_val       = 0b00000111,
            .first_byte_mask      = 0b11100111
        }
    },

    {
        .cls  = e_ic_xchg,
        .var  = e_iv_rm_with_reg,
        .desc = {
            .text                 = "XCHG Register/memory with register",
            .requires_second_byte = false,
            .first_byte_val       = 0b10000110,
            .first_byte_mask      = 0b11111110
        }
    },
    {
        .cls  = e_ic_xchg,
        .var  = e_iv_reg_with_acc,
        .desc = {
            .text                 = "XCHG Register with accumulator",
            .requires_second_byte = false,
            .first_byte_val       = 0b10010000,
            .first_byte_mask      = 0b11111000
        }
    },

    {
        .cls  = e_ic_in,
        .var  = e_iv_fixed_port,
        .desc = {
            .text                 = "IN Fixed port",
            .requires_second_byte = false,
            .first_byte_val       = 0b11100100,
            .first_byte_mask      = 0b11111110
        }
    },
    {
        .cls  = e_ic_in,
        .var  = e_iv_variable_port,
        .desc = {
            .text                 = "IN Variable port",
            .requires_second_byte = false,
            .first_byte_val       = 0b11101100,
            .first_byte_mask      = 0b11111110
        }
    },
    {
        .cls  = e_ic_out,
        .var  = e_iv_fixed_port,
        .desc = {
            .text                 = "OUT Fixed port",
            .requires_second_byte = false,
            .first_byte_val       = 0b11100100,
            .first_byte_mask      = 0b11111110
        }
    },
    {
        .cls  = e_ic_out,
        .var  = e_iv_variable_port,
        .desc = {
            .text                 = "OUT Variable port",
            .requires_second_byte = false,
            .first_byte_val       = 0b11101100,
            .first_byte_mask      = 0b11111110
        }
    },

    {
        .cls  = e_ic_xlat,
        .var  = e_iv_none,
        .desc = {
            .text                 = "XLAT",
            .requires_second_byte = false,
            .first_byte_val       = 0b11010111,
            .first_byte_mask      = 0b11111111
        }
    },
    {
        .cls  = e_ic_lea,
        .var  = e_iv_none,
        .desc = {
            .text                 = "LEA",
            .requires_second_byte = false,
            .first_byte_val       = 0b10001101,
            .first_byte_mask      = 0b11111111
        }
    },
    {
        .cls  = e_ic_lds,
        .var  = e_iv_none,
        .desc = {
            .text                 = "LDS",
            .requires_second_byte = false,
            .first_byte_val       = 0b11000101,
            .first_byte_mask      = 0b11111111
        }
    },
    {
        .cls  = e_ic_les,
        .var  = e_iv_none,
        .desc = {
            .text                 = "LES",
            .requires_second_byte = false,
            .first_byte_val       = 0b11000100,
            .first_byte_mask      = 0b11111111
        }
    },
    {
        .cls  = e_ic_lahf,
        .var  = e_iv_none,
        .desc = {
            .text                 = "LAHF",
            .requires_second_byte = false,
            .first_byte_val       = 0b10011111,
            .first_byte_mask      = 0b11111111
        }
    },
    {
        .cls  = e_ic_sahf,
        .var  = e_iv_none,
        .desc = {
            .text                 = "SAHF",
            .requires_second_byte = false,
            .first_byte_val       = 0b10011110,
            .first_byte_mask      = 0b11111111
        }
    },
    {
        .cls  = e_ic_pushf,
        .var  = e_iv_none,
        .desc = {
            .text                 = "PUSHF",
            .requires_second_byte = false,
            .first_byte_val       = 0b10011100,
            .first_byte_mask      = 0b11111111
        }
    },
    {
        .cls  = e_ic_popf,
        .var  = e_iv_none,
        .desc = {
            .text                 = "POPF",
            .requires_second_byte = false,
            .first_byte_val       = 0b10011101,
            .first_byte_mask      = 0b11111111
        }
    },

    {
        .cls  = e_ic_add,
        .var  = e_iv_rm_with_reg,
        .desc = {
            .text                 = "ADD Register/memory + register to either",
            .requires_second_byte = false,
            .first_byte_val       = 0b00000000,
            .first_byte_mask      = 0b11111100
        }
    },
    {
        .cls  = e_ic_add,
        .var  = e_iv_imm_to_rm,
        .desc = {
            .text                 = "ADD Immediate to register/memory",
            .requires_second_byte = true,
            .first_byte_val       = 0b10000000,
            .first_byte_mask      = 0b11111100,
            .second_byte_val      = 0b00000000,
            .second_byte_mask     = 0b00111000
        }
    },
    {
        .cls  = e_ic_add,
        .var  = e_iv_imm_to_acc,
        .desc = {
            .text                 = "ADD Immediate to accumulator",
            .requires_second_byte = false,
            .first_byte_val       = 0b00000100,
            .first_byte_mask      = 0b11111110
        }
    },
    {
        .cls  = e_ic_adc,
        .var  = e_iv_rm_with_reg,
        .desc = {
            .text                 = "ADC Register/memory + register to either",
            .requires_second_byte = false,
            .first_byte_val       = 0b00010000,
            .first_byte_mask      = 0b11111100
        }
    },
    {
        .cls  = e_ic_adc,
        .var  = e_iv_imm_to_rm,
        .desc = {
            .text                 = "ADC Immediate to register/memory",
            .requires_second_byte = true,
            .first_byte_val       = 0b10000000,
            .first_byte_mask      = 0b11111100,
            .second_byte_val      = 0b00010000,
            .second_byte_mask     = 0b00111000
        }
    },
    {
        .cls  = e_ic_adc,
        .var  = e_iv_imm_to_acc,
        .desc = {
            .text                 = "ADC Immediate to accumulator",
            .requires_second_byte = false,
            .first_byte_val       = 0b00010100,
            .first_byte_mask      = 0b11111110
        }
    },

    {
        .cls  = e_ic_inc,
        .var  = e_iv_rm,
        .desc = {
            .text                 = "INC Register/memory",
            .requires_second_byte = true,
            .first_byte_val       = 0b11111111,
            .first_byte_mask      = 0b11111110,
            .second_byte_val      = 0b00000000,
            .second_byte_mask     = 0b00111000
        }
    },
    {
        .cls  = e_ic_inc,
        .var  = e_iv_reg,
        .desc = {
            .text                 = "INC Register",
            .requires_second_byte = false,
            .first_byte_val       = 0b01000000,
            .first_byte_mask      = 0b11111000
        }
    },

    {
        .cls  = e_ic_aaa,
        .var  = e_iv_none,
        .desc = {
            .text                 = "AAA",
            .requires_second_byte = false,
            .first_byte_val       = 0b00110111,
            .first_byte_mask      = 0b11111111,
        }
    },
    {
        .cls  = e_ic_daa,
        .var  = e_iv_none,
        .desc = {
            .text                 = "DAA",
            .requires_second_byte = false,
            .first_byte_val       = 0b00100111,
            .first_byte_mask      = 0b11111111,
        }
    },

    {
        .cls  = e_ic_sub,
        .var  = e_iv_rm_with_reg,
        .desc = {
            .text                 = "SUB Register/memory - register from either",
            .requires_second_byte = false,
            .first_byte_val       = 0b00101000,
            .first_byte_mask      = 0b11111100
        }
    },
    {
        .cls  = e_ic_sub,
        .var  = e_iv_imm_to_rm,
        .desc = {
            .text                 = "SUB Immediate from register/memory",
            .requires_second_byte = true,
            .first_byte_val       = 0b10000000,
            .first_byte_mask      = 0b11111100,
            .second_byte_val      = 0b00101000,
            .second_byte_mask     = 0b00111000
        }
    },
    {
        .cls  = e_ic_sub,
        .var  = e_iv_imm_to_acc,
        .desc = {
            .text                 = "SUB Immediate from accumulator",
            .requires_second_byte = false,
            .first_byte_val       = 0b00101100,
            .first_byte_mask      = 0b11111110
        }
    },
    {
        .cls  = e_ic_sbb,
        .var  = e_iv_rm_with_reg,
        .desc = {
            .text                 = "SBB Register/memory - register from either",
            .requires_second_byte = false,
            .first_byte_val       = 0b00011000,
            .first_byte_mask      = 0b11111100
        }
    },
    {
        .cls  = e_ic_sbb,
        .var  = e_iv_imm_to_rm,
        .desc = {
            .text                 = "SBB Immediate from register/memory",
            .requires_second_byte = true,
            .first_byte_val       = 0b10000000,
            .first_byte_mask      = 0b11111100,
            .second_byte_val      = 0b00011000,
            .second_byte_mask     = 0b00111000
        }
    },
    {
        .cls  = e_ic_sbb,
        .var  = e_iv_imm_to_acc,
        .desc = {
            .text                 = "SBB Immediate from accumulator",
            .requires_second_byte = false,
            .first_byte_val       = 0b00011100,
            .first_byte_mask      = 0b11111110
        }
    },

    {
        .cls  = e_ic_dec,
        .var  = e_iv_rm,
        .desc = {
            .text                 = "DEC Register/memory",
            .requires_second_byte = true,
            .first_byte_val       = 0b11111111,
            .first_byte_mask      = 0b11111110,
            .second_byte_val      = 0b00001000,
            .second_byte_mask     = 0b00111000
        }
    },
    {
        .cls  = e_ic_dec,
        .var  = e_iv_reg,
        .desc = {
            .text                 = "DEC Register",
            .requires_second_byte = false,
            .first_byte_val       = 0b01001000,
            .first_byte_mask      = 0b11111000
        }
    },

    {
        .cls  = e_ic_neg,
        .var  = e_iv_none,
        .desc = {
            .text                 = "NEG",
            .requires_second_byte = true,
            .first_byte_val       = 0b11110110,
            .first_byte_mask      = 0b11111110,
            .second_byte_val      = 0b00011000,
            .second_byte_mask     = 0b00111000
        }
    },

    {
        .cls  = e_ic_cmp,
        .var  = e_iv_rm_with_reg,
        .desc = {
            .text                 = "CMP Register/memory and register",
            .requires_second_byte = false,
            .first_byte_val       = 0b00111000,
            .first_byte_mask      = 0b11111100
        }
    },
    {
        .cls  = e_ic_cmp,
        .var  = e_iv_imm_to_rm,
        .desc = {
            .text                 = "CMP Immediate and register/memory",
            .requires_second_byte = true,
            .first_byte_val       = 0b10000000,
            .first_byte_mask      = 0b11111100,
            .second_byte_val      = 0b00111000,
            .second_byte_mask     = 0b00111000
        }
    },
    {
        .cls  = e_ic_cmp,
        .var  = e_iv_imm_to_acc,
        .desc = {
            .text                 = "CMP Immediate and accumulator",
            .requires_second_byte = false,
            .first_byte_val       = 0b00111100,
            .first_byte_mask      = 0b11111110
        }
    },

    {
        .cls  = e_ic_aas,
        .var  = e_iv_none,
        .desc = {
            .text                 = "AAS",
            .requires_second_byte = false,
            .first_byte_val       = 0b00111111,
            .first_byte_mask      = 0b11111111
        }
    },
    {
        .cls  = e_ic_das,
        .var  = e_iv_none,
        .desc = {
            .text                 = "DAS",
            .requires_second_byte = false,
            .first_byte_val       = 0b00101111,
            .first_byte_mask      = 0b11111111
        }
    },
    {
        .cls  = e_ic_mul,
        .var  = e_iv_none,
        .desc = {
            .text                 = "MUL",
            .requires_second_byte = true,
            .first_byte_val       = 0b11110110,
            .first_byte_mask      = 0b11111110,
            .second_byte_val      = 0b00100000,
            .second_byte_mask     = 0b00111000
        }
    },
    {
        .cls  = e_ic_imul,
        .var  = e_iv_none,
        .desc = {
            .text                 = "IMUL",
            .requires_second_byte = true,
            .first_byte_val       = 0b11110110,
            .first_byte_mask      = 0b11111110,
            .second_byte_val      = 0b00101000,
            .second_byte_mask     = 0b00111000
        }
    },
    {
        .cls  = e_ic_aam,
        .var  = e_iv_none,
        .desc = {
            .text                 = "AAM",
            .requires_second_byte = true,
            .first_byte_val       = 0b11010100,
            .first_byte_mask      = 0b11111111,
            .second_byte_val      = 0b00001010,
            .second_byte_mask     = 0b11111111
        }
    },
    {
        .cls  = e_ic_div,
        .var  = e_iv_none,
        .desc = {
            .text                 = "DIV",
            .requires_second_byte = true,
            .first_byte_val       = 0b11110110,
            .first_byte_mask      = 0b11111110,
            .second_byte_val      = 0b00110000,
            .second_byte_mask     = 0b00111000
        }
    },
    {
        .cls  = e_ic_idiv,
        .var  = e_iv_none,
        .desc = {
            .text                 = "IDIV",
            .requires_second_byte = true,
            .first_byte_val       = 0b11110110,
            .first_byte_mask      = 0b11111110,
            .second_byte_val      = 0b00111000,
            .second_byte_mask     = 0b00111000
        }
    },
    {
        .cls  = e_ic_aad,
        .var  = e_iv_none,
        .desc = {
            .text                 = "AAD",
            .requires_second_byte = true,
            .first_byte_val       = 0b11010101,
            .first_byte_mask      = 0b11111111,
            .second_byte_val      = 0b00001010,
            .second_byte_mask     = 0b11111111
        }
    },
    {
        .cls  = e_ic_cbw,
        .var  = e_iv_none,
        .desc = {
            .text                 = "CBW",
            .requires_second_byte = false,
            .first_byte_val       = 0b10011000,
            .first_byte_mask      = 0b11111111
        }
    },
    {
        .cls  = e_ic_cwd,
        .var  = e_iv_none,
        .desc = {
            .text                 = "CWD",
            .requires_second_byte = false,
            .first_byte_val       = 0b10011001,
            .first_byte_mask      = 0b11111111
        }
    },

    {
        .cls  = e_ic_not,
        .var  = e_iv_none,
        .desc = {
            .text                 = "NOT",
            .requires_second_byte = true,
            .first_byte_val       = 0b11110110,
            .first_byte_mask      = 0b11111110,
            .second_byte_val      = 0b00010000,
            .second_byte_mask     = 0b00111000
        }
    },
    {
        .cls  = e_ic_shl,
        .var  = e_iv_none,
        .desc = {
            .text                 = "SHL/SAL",
            .requires_second_byte = true,
            .first_byte_val       = 0b11010000,
            .first_byte_mask      = 0b11111100,
            .second_byte_val      = 0b00100000,
            .second_byte_mask     = 0b00111000
        }
    },
    {
        .cls  = e_ic_shr,
        .var  = e_iv_none,
        .desc = {
            .text                 = "SHR",
            .requires_second_byte = true,
            .first_byte_val       = 0b11010000,
            .first_byte_mask      = 0b11111100,
            .second_byte_val      = 0b00101000,
            .second_byte_mask     = 0b00111000
        }
    },
    {
        .cls  = e_ic_sar,
        .var  = e_iv_none,
        .desc = {
            .text                 = "SAR",
            .requires_second_byte = true,
            .first_byte_val       = 0b11010000,
            .first_byte_mask      = 0b11111100,
            .second_byte_val      = 0b00111000,
            .second_byte_mask     = 0b00111000
        }
    },
    {
        .cls  = e_ic_rol,
        .var  = e_iv_none,
        .desc = {
            .text                 = "ROL",
            .requires_second_byte = true,
            .first_byte_val       = 0b11010000,
            .first_byte_mask      = 0b11111100,
            .second_byte_val      = 0b00000000,
            .second_byte_mask     = 0b00111000
        }
    },
    {
        .cls  = e_ic_ror,
        .var  = e_iv_none,
        .desc = {
            .text                 = "ROR",
            .requires_second_byte = true,
            .first_byte_val       = 0b11010000,
            .first_byte_mask      = 0b11111100,
            .second_byte_val      = 0b00001000,
            .second_byte_mask     = 0b00111000
        }
    },
    {
        .cls  = e_ic_rcl,
        .var  = e_iv_none,
        .desc = {
            .text                 = "RCL",
            .requires_second_byte = true,
            .first_byte_val       = 0b11010000,
            .first_byte_mask      = 0b11111100,
            .second_byte_val      = 0b00010000,
            .second_byte_mask     = 0b00111000
        }
    },
    {
        .cls  = e_ic_rcr,
        .var  = e_iv_none,
        .desc = {
            .text                 = "RCR",
            .requires_second_byte = true,
            .first_byte_val       = 0b11010000,
            .first_byte_mask      = 0b11111100,
            .second_byte_val      = 0b00011000,
            .second_byte_mask     = 0b00111000
        }
    },

    {
        .cls  = e_ic_and,
        .var  = e_iv_rm_with_reg,
        .desc = {
            .text                 = "AND Register/memory and register to either",
            .requires_second_byte = false,
            .first_byte_val       = 0b00100000,
            .first_byte_mask      = 0b11111100
        }
    },
    {
        .cls  = e_ic_and,
        .var  = e_iv_imm_to_rm,
        .desc = {
            .text                 = "AND Immediate to register/memory",
            .requires_second_byte = true,
            .first_byte_val       = 0b10000000,
            .first_byte_mask      = 0b11111110,
            .second_byte_val      = 0b00100000,
            .second_byte_mask     = 0b00111000
        }
    },
    {
        .cls  = e_ic_and,
        .var  = e_iv_imm_to_acc,
        .desc = {
            .text                 = "AND Immediate to accumulator",
            .requires_second_byte = false,
            .first_byte_val       = 0b00100100,
            .first_byte_mask      = 0b11111110
        }
    },

    {
        .cls  = e_ic_test,
        .var  = e_iv_rm_with_reg,
        .desc = {
            .text                 = "TEST Register/memory and register",
            .requires_second_byte = false,
            .first_byte_val       = 0b00010000,
            .first_byte_mask      = 0b11111100
        }
    },
    {
        .cls  = e_ic_test,
        .var  = e_iv_imm_to_rm,
        .desc = {
            .text                 = "TEST Immediate and register/memory",
            .requires_second_byte = true,
            .first_byte_val       = 0b11110110,
            .first_byte_mask      = 0b11111110,
            .second_byte_val      = 0b00000000,
            .second_byte_mask     = 0b00111000
        }
    },
    {
        .cls  = e_ic_test,
        .var  = e_iv_imm_to_acc,
        .desc = {
            .text                 = "TEST Immediate and accumulator",
            .requires_second_byte = false,
            .first_byte_val       = 0b10101000,
            .first_byte_mask      = 0b11111110
        }
    },

    {
        .cls  = e_ic_or,
        .var  = e_iv_rm_with_reg,
        .desc = {
            .text                 = "OR Register/memory and register to either",
            .requires_second_byte = false,
            .first_byte_val       = 0b00001000,
            .first_byte_mask      = 0b11111100
        }
    },
    {
        .cls  = e_ic_or,
        .var  = e_iv_imm_to_rm,
        .desc = {
            .text                 = "OR Immediate to register/memory",
            .requires_second_byte = true,
            .first_byte_val       = 0b10000000,
            .first_byte_mask      = 0b11111110,
            .second_byte_val      = 0b00001000,
            .second_byte_mask     = 0b00111000
        }
    },
    {
        .cls  = e_ic_or,
        .var  = e_iv_imm_to_acc,
        .desc = {
            .text                 = "OR Immediate to accumulator",
            .requires_second_byte = false,
            .first_byte_val       = 0b00001100,
            .first_byte_mask      = 0b11111110
        }
    },

    {
        .cls  = e_ic_xor,
        .var  = e_iv_rm_with_reg,
        .desc = {
            .text                 = "XOR Register/memory and register to either",
            .requires_second_byte = false,
            .first_byte_val       = 0b00110000,
            .first_byte_mask      = 0b11111100
        }
    },
    {
        .cls  = e_ic_xor,
        .var  = e_iv_imm_to_rm,
        .desc = {
            .text                 = "XOR Immediate to register/memory",
            .requires_second_byte = false,
            .first_byte_val       = 0b00110100,
            .first_byte_mask      = 0b11111110
        }
    },
    {
        .cls  = e_ic_xor,
        .var  = e_iv_imm_to_acc,
        .desc = {
            .text                 = "XOR Immediate to accumulator",
            .requires_second_byte = false,
            .first_byte_val       = 0b00110100, // @TODO: wtf is this? How do i tell this apart from prev case?
            .first_byte_mask      = 0b11111110
        }
    },

    {
        .cls  = e_ic_rep,
        .var  = e_iv_none,
        .desc = {
            .text                 = "REP",
            .requires_second_byte = false,
            .first_byte_val       = 0b11110010,
            .first_byte_mask      = 0b11111110
        }
    },
    {
        .cls  = e_ic_movs,
        .var  = e_iv_none,
        .desc = {
            .text                 = "MOVS",
            .requires_second_byte = false,
            .first_byte_val       = 0b10100100,
            .first_byte_mask      = 0b11111110
        }
    },
    {
        .cls  = e_ic_cmps,
        .var  = e_iv_none,
        .desc = {
            .text                 = "CMPS",
            .requires_second_byte = false,
            .first_byte_val       = 0b10100110,
            .first_byte_mask      = 0b11111110
        }
    },
    {
        .cls  = e_ic_scas,
        .var  = e_iv_none,
        .desc = {
            .text                 = "SCAS",
            .requires_second_byte = false,
            .first_byte_val       = 0b10101110,
            .first_byte_mask      = 0b11111110
        }
    },
    {
        .cls  = e_ic_lods,
        .var  = e_iv_none,
        .desc = {
            .text                 = "LODS",
            .requires_second_byte = false,
            .first_byte_val       = 0b10101100,
            .first_byte_mask      = 0b11111110
        }
    },
    {
        .cls  = e_ic_stos,
        .var  = e_iv_none,
        .desc = {
            .text                 = "STOS",
            .requires_second_byte = false,
            .first_byte_val       = 0b10101010,
            .first_byte_mask      = 0b11111110
        }
    },

    {
        .cls  = e_ic_call,
        .var  = e_iv_direct_within_segment,
        .desc = {
            .text                 = "CALL Direct within segment",
            .requires_second_byte = false,
            .first_byte_val       = 0b11101000,
            .first_byte_mask      = 0b11111111
        }
    },
    {
        .cls  = e_ic_call,
        .var  = e_iv_indirect_within_segment,
        .desc = {
            .text                 = "CALL Indirect within segment",
            .requires_second_byte = true,
            .first_byte_val       = 0b11111111,
            .first_byte_mask      = 0b11111111,
            .second_byte_val      = 0b00010000,
            .second_byte_mask     = 0b00111000
        }
    },
    {
        .cls  = e_ic_call,
        .var  = e_iv_direct_intersegment,
        .desc = {
            .text                 = "CALL Direct intersegment",
            .requires_second_byte = false,
            .first_byte_val       = 0b10011010,
            .first_byte_mask      = 0b11111111
        }
    },
    {
        .cls  = e_ic_call,
        .var  = e_iv_indirect_within_segment,
        .desc = {
            .text                 = "CALL Indirect intersegment",
            .requires_second_byte = true,
            .first_byte_val       = 0b11111111,
            .first_byte_mask      = 0b11111111,
            .second_byte_val      = 0b00011000,
            .second_byte_mask     = 0b00111000
        }
    },

    {
        .cls  = e_ic_jmp,
        .var  = e_iv_direct_within_segment,
        .desc = {
            .text                 = "JMP Direct within segment",
            .requires_second_byte = false,
            .first_byte_val       = 0b11101001,
            .first_byte_mask      = 0b11111111
        }
    },
    {
        .cls  = e_ic_jmp,
        .var  = e_iv_direct_within_segment_short,
        .desc = {
            .text                 = "JMP Direct within segment-short",
            .requires_second_byte = false,
            .first_byte_val       = 0b11101011,
            .first_byte_mask      = 0b11111111
        }
    },
    {
        .cls  = e_ic_jmp,
        .var  = e_iv_indirect_within_segment,
        .desc = {
            .text                 = "JMP Indirect within segment",
            .requires_second_byte = true,
            .first_byte_val       = 0b11111111,
            .first_byte_mask      = 0b11111111,
            .second_byte_val      = 0b00100000,
            .second_byte_mask     = 0b00111000
        }
    },
    {
        .cls  = e_ic_jmp,
        .var  = e_iv_direct_intersegment,
        .desc = {
            .text                 = "JMP Direct intersegment",
            .requires_second_byte = false,
            .first_byte_val       = 0b11101010,
            .first_byte_mask      = 0b11111111
        }
    },
    {
        .cls  = e_ic_jmp,
        .var  = e_iv_indirect_within_segment,
        .desc = {
            .text                 = "JMP Indirect intersegment",
            .requires_second_byte = true,
            .first_byte_val       = 0b11111111,
            .first_byte_mask      = 0b11111111,
            .second_byte_val      = 0b00101000,
            .second_byte_mask     = 0b00111000
        }
    },

    {
        .cls  = e_ic_ret,
        .var  = e_iv_within_segment,
        .desc = {
            .text                 = "RET Within segment",
            .requires_second_byte = false,
            .first_byte_val       = 0b11000011,
            .first_byte_mask      = 0b11111111
        }
    },
    {
        .cls  = e_ic_ret,
        .var  = e_iv_within_segment_add_imm_to_sp,
        .desc = {
            .text                 = "RET Within segment adding immed to SP",
            .requires_second_byte = false,
            .first_byte_val       = 0b11000010,
            .first_byte_mask      = 0b11111111
        }
    },
    {
        .cls  = e_ic_ret,
        .var  = e_iv_intersegment,
        .desc = {
            .text                 = "RET Intersegment",
            .requires_second_byte = false,
            .first_byte_val       = 0b11001011,
            .first_byte_mask      = 0b11111111
        }
    },
    {
        .cls  = e_ic_ret,
        .var  = e_iv_intersegment_add_imm_to_sp,
        .desc = {
            .text                 = "RET Intersegment adding immed to SP",
            .requires_second_byte = false,
            .first_byte_val       = 0b11001010,
            .first_byte_mask      = 0b11111111
        }
    },

    // @TODO: alt names for j texts
    {
        .cls  = e_ic_je,
        .var  = e_iv_none,
        .desc = {
            .text                 = "JE",
            .requires_second_byte = false,
            .first_byte_val       = 0b01110100,
            .first_byte_mask      = 0b11111111
        }
    },
    {
        .cls  = e_ic_jl,
        .var  = e_iv_none,
        .desc = {
            .text                 = "JL",
            .requires_second_byte = false,
            .first_byte_val       = 0b01111100,
            .first_byte_mask      = 0b11111111
        }
    },
    {
        .cls  = e_ic_jle,
        .var  = e_iv_none,
        .desc = {
            .text                 = "JLE",
            .requires_second_byte = false,
            .first_byte_val       = 0b01111110,
            .first_byte_mask      = 0b11111111
        }
    },
    {
        .cls  = e_ic_jb,
        .var  = e_iv_none,
        .desc = {
            .text                 = "JB",
            .requires_second_byte = false,
            .first_byte_val       = 0b01110010,
            .first_byte_mask      = 0b11111111
        }
    },
    {
        .cls  = e_ic_jbe,
        .var  = e_iv_none,
        .desc = {
            .text                 = "JBE",
            .requires_second_byte = false,
            .first_byte_val       = 0b01110110,
            .first_byte_mask      = 0b11111111
        }
    },
    {
        .cls  = e_ic_jp,
        .var  = e_iv_none,
        .desc = {
            .text                 = "JP",
            .requires_second_byte = false,
            .first_byte_val       = 0b01111010,
            .first_byte_mask      = 0b11111111
        }
    },
    {
        .cls  = e_ic_jo,
        .var  = e_iv_none,
        .desc = {
            .text                 = "JO",
            .requires_second_byte = false,
            .first_byte_val       = 0b01110000,
            .first_byte_mask      = 0b11111111
        }
    },
    {
        .cls  = e_ic_js,
        .var  = e_iv_none,
        .desc = {
            .text                 = "JS",
            .requires_second_byte = false,
            .first_byte_val       = 0b01111000,
            .first_byte_mask      = 0b11111111
        }
    },
    {
        .cls  = e_ic_jne,
        .var  = e_iv_none,
        .desc = {
            .text                 = "JNE",
            .requires_second_byte = false,
            .first_byte_val       = 0b01110101,
            .first_byte_mask      = 0b11111111
        }
    },
    {
        .cls  = e_ic_jge,
        .var  = e_iv_none,
        .desc = {
            .text                 = "JGE",
            .requires_second_byte = false,
            .first_byte_val       = 0b01111101,
            .first_byte_mask      = 0b11111111
        }
    },
    {
        .cls  = e_ic_jg,
        .var  = e_iv_none,
        .desc = {
            .text                 = "JG",
            .requires_second_byte = false,
            .first_byte_val       = 0b01111111,
            .first_byte_mask      = 0b11111111
        }
    },
    {
        .cls  = e_ic_jae,
        .var  = e_iv_none,
        .desc = {
            .text                 = "JAE",
            .requires_second_byte = false,
            .first_byte_val       = 0b01110011,
            .first_byte_mask      = 0b11111111
        }
    },
    {
        .cls  = e_ic_ja,
        .var  = e_iv_none,
        .desc = {
            .text                 = "JA",
            .requires_second_byte = false,
            .first_byte_val       = 0b01110111,
            .first_byte_mask      = 0b11111111
        }
    },
    {
        .cls  = e_ic_jnp,
        .var  = e_iv_none,
        .desc = {
            .text                 = "JNP",
            .requires_second_byte = false,
            .first_byte_val       = 0b01111011,
            .first_byte_mask      = 0b11111111
        }
    },
    {
        .cls  = e_ic_jno,
        .var  = e_iv_none,
        .desc = {
            .text                 = "JNO",
            .requires_second_byte = false,
            .first_byte_val       = 0b01110001,
            .first_byte_mask      = 0b11111111
        }
    },
    {
        .cls  = e_ic_jns,
        .var  = e_iv_none,
        .desc = {
            .text                 = "JNS",
            .requires_second_byte = false,
            .first_byte_val       = 0b01111001,
            .first_byte_mask      = 0b11111111
        }
    },

    {
        .cls  = e_ic_loop,
        .var  = e_iv_none,
        .desc = {
            .text                 = "LOOP",
            .requires_second_byte = false,
            .first_byte_val       = 0b11100010,
            .first_byte_mask      = 0b11111111
        }
    },
    {
        .cls  = e_ic_loope,
        .var  = e_iv_none,
        .desc = {
            .text                 = "LOOPZ/LOOPE",
            .requires_second_byte = false,
            .first_byte_val       = 0b11100001,
            .first_byte_mask      = 0b11111111
        }
    },
    {
        .cls  = e_ic_loopne,
        .var  = e_iv_none,
        .desc = {
            .text                 = "LOOPNZ/LOOPNE",
            .requires_second_byte = false,
            .first_byte_val       = 0b11100000,
            .first_byte_mask      = 0b11111111
        }
    },
    {
        .cls  = e_ic_jcxz,
        .var  = e_iv_none,
        .desc = {
            .text                 = "JCXZ",
            .requires_second_byte = false,
            .first_byte_val       = 0b11100011,
            .first_byte_mask      = 0b11111111
        }
    },

    {
        .cls  = e_ic_int,
        .var  = e_iv_int_type_specified,
        .desc = {
            .text                 = "INT Type specified",
            .requires_second_byte = false,
            .first_byte_val       = 0b11001101,
            .first_byte_mask      = 0b11111111
        }
    },

    {
        .cls  = e_ic_int,
        .var  = e_iv_int_type_specified,
        .desc = {
            .text                 = "INT Type specified",
            .requires_second_byte = false,
            .first_byte_val       = 0b11001101,
            .first_byte_mask      = 0b11111111
        }
    },
    {
        .cls  = e_ic_int,
        .var  = e_iv_int_type_unspecified,
        .desc = {
            .text                 = "INT Type unspecified", // @TODO: check that the manual says this
            .requires_second_byte = false,
            .first_byte_val       = 0b11001100,
            .first_byte_mask      = 0b11111111
        }
    },
    {
        .cls  = e_ic_into,
        .var  = e_iv_none,
        .desc = {
            .text                 = "INTO",
            .requires_second_byte = false,
            .first_byte_val       = 0b11001110,
            .first_byte_mask      = 0b11111111
        }
    },
    {
        .cls  = e_ic_iret,
        .var  = e_iv_none,
        .desc = {
            .text                 = "IRET",
            .requires_second_byte = false,
            .first_byte_val       = 0b11001111,
            .first_byte_mask      = 0b11111111
        }
    },

    {
        .cls  = e_ic_clc,
        .var  = e_iv_none,
        .desc = {
            .text                 = "CLC",
            .requires_second_byte = false,
            .first_byte_val       = 0b11111000,
            .first_byte_mask      = 0b11111111
        }
    },
    {
        .cls  = e_ic_cmc,
        .var  = e_iv_none,
        .desc = {
            .text                 = "CMC",
            .requires_second_byte = false,
            .first_byte_val       = 0b11110101,
            .first_byte_mask      = 0b11111111
        }
    },
    {
        .cls  = e_ic_stc,
        .var  = e_iv_none,
        .desc = {
            .text                 = "STC",
            .requires_second_byte = false,
            .first_byte_val       = 0b11111001,
            .first_byte_mask      = 0b11111111
        }
    },
    {
        .cls  = e_ic_cld,
        .var  = e_iv_none,
        .desc = {
            .text                 = "CLD",
            .requires_second_byte = false,
            .first_byte_val       = 0b11111100,
            .first_byte_mask      = 0b11111111
        }
    },
    {
        .cls  = e_ic_std,
        .var  = e_iv_none,
        .desc = {
            .text                 = "STD",
            .requires_second_byte = false,
            .first_byte_val       = 0b11111101,
            .first_byte_mask      = 0b11111111
        }
    },
    {
        .cls  = e_ic_cli,
        .var  = e_iv_none,
        .desc = {
            .text                 = "CLI",
            .requires_second_byte = false,
            .first_byte_val       = 0b11111010,
            .first_byte_mask      = 0b11111111
        }
    },
    {
        .cls  = e_ic_sti,
        .var  = e_iv_none,
        .desc = {
            .text                 = "STI",
            .requires_second_byte = false,
            .first_byte_val       = 0b11111011,
            .first_byte_mask      = 0b11111111
        }
    },
    {
        .cls  = e_ic_hlt,
        .var  = e_iv_none,
        .desc = {
            .text                 = "HLT",
            .requires_second_byte = false,
            .first_byte_val       = 0b11110100,
            .first_byte_mask      = 0b11111111
        }
    },
    {
        .cls  = e_ic_wait,
        .var  = e_iv_none,
        .desc = {
            .text                 = "WAIT",
            .requires_second_byte = false,
            .first_byte_val       = 0b10011011,
            .first_byte_mask      = 0b11111111
        }
    },
    {
        .cls  = e_ic_esc,
        .var  = e_iv_none,
        .desc = {
            .text                 = "ESC",
            .requires_second_byte = false,
            .first_byte_val       = 0b11011000,
            .first_byte_mask      = 0b11111000
        }
    },
    {
        .cls  = e_ic_lock,
        .var  = e_iv_none,
        .desc = {
            .text                 = "LOCK",
            .requires_second_byte = false,
            .first_byte_val       = 0b11110000,
            .first_byte_mask      = 0b11111111
        }
    },
    {
        .cls  = e_ic_segment,
        .var  = e_iv_none,
        .desc = {
            .text                 = "SEGMENT",
            .requires_second_byte = false,
            .first_byte_val       = 0b00100110,
            .first_byte_mask      = 0b11100111
        }
    },
};

static bool g_machine_is_big_endian;

inline uint16_t u16_swap_bytes(uint16_t n)
{
    return (n >> 8) | ((n & 0xFF) << 8);
}

template <class T>
bool fetch_mem(FILE *f, T *out)
{
    return fread(out, sizeof(*out), 1, f) == 1;
}

template <class T>
bool fetch_integer_with_endian_swap(FILE *f, T *out)
{
    static_assert(sizeof(*out) <= 2, "Endian swap fetch only for 8 and 16 bit integers");

    if (fread(out, sizeof(*out), 1, f) != 1)
        return false;
    if (sizeof(*out) > 1 && !g_machine_is_big_endian)
        *out = u16_swap_bytes(*out);
    return true;
}

#define TRY_ELSE_RETURN(_try, _retval) \
    do {                               \
        if (!(_try))                   \
            return _retval;            \
    } while (0)

#define NOT_IMPL() fprintf(stderr, "Not implemented\n")

#define ARR_SIZE(_arr) (sizeof(_arr) / sizeof((_arr)[0]))

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

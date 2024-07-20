#pragma once
#include "defs.hpp"
#include "instruction.hpp"

enum instr_bits_t {
    e_bits_end = 0, // denotes end of bit field list

    e_bits_literal,

    e_bits_w,  
    e_bits_d,  
    e_bits_s,  
    e_bits_z,  
    e_bits_v,  

    e_bits_mod,
    e_bits_reg,
    e_bits_rm, 
    e_bits_sr, 

    e_bits_disp,
    e_bits_disp_always_w,
    e_bits_data,
    e_bits_data_w_if_w,
    e_bits_rm_always_w,
    e_bits_jmp_rel_disp,
    e_bits_far,

    e_bits_ext_opcode_lo,
    e_bits_ext_opcode_hi,

    e_bits_max
};

struct instr_bit_field_t {
    instr_bits_t type;
    u32 bit_count;
    u8 val;
};

struct instruction_encoding_t {
    op_t op;
    instr_bit_field_t fields[16] = {}; 
};

struct instruction_table_t {
    const instruction_encoding_t **table;
    u32 size;
    u16 mask; // we always check two bytes
};

instruction_table_t build_instruction_table();

#pragma once
#include "defs.hpp"

enum op_t {
    e_op_invalid = 0,

#define INST(_op, ...) e_op_##_op,
#define INSTALT(...)

#include "instruction_table.cpp.inl"

    e_op_max
};

enum reg_t {
    e_reg_a = 0,
    e_reg_b,
    e_reg_c,
    e_reg_d,

    e_reg_sp,
    e_reg_bp,
    e_reg_si,
    e_reg_di,

    e_reg_es,
    e_reg_cs,
    e_reg_ss,
    e_reg_ds,

    e_reg_ip,
    e_reg_flags,

    e_reg_max
};
  
enum ea_base_t {
    e_ea_base_bx_si = 0,
    e_ea_base_bx_di,
    e_ea_base_bp_si,
    e_ea_base_bp_di,

    e_ea_base_si,
    e_ea_base_di,
    e_ea_base_bp,
    e_ea_base_bx,

    e_ea_base_direct,

    e_ea_base_max
};

enum instr_flags_t : u32 {
    e_iflags_none = 0,

    e_iflags_w    = 1 << 0,
    e_iflags_s    = 1 << 1,
    e_iflags_z    = 1 << 2,
    e_iflags_lock = 1 << 3,
    e_iflags_rep  = 1 << 4,

    e_iflags_seg_override = 1 << 5,

    e_iflags_imm_is_rel_disp = 1 << 6,

    e_iflags_far = 1 << 7
};

struct reg_access_t {
    reg_t reg;
    u32 offset;
    u32 size;
};

struct ea_mem_access_t {
    ea_base_t base;
    i16 disp; 
};

struct cs_ip_pair_t {
    u16 cs;
    u16 ip;
};

enum operand_type_t {
    e_operand_none = 0,

    e_operand_reg,
    e_operand_mem,
    e_operand_imm,
    e_operand_cs_ip
};

struct operand_t {
    operand_type_t type;
    union {
        reg_access_t reg;
        ea_mem_access_t mem;
        u16 imm;
        cs_ip_pair_t cs_ip;
    } data;
};

struct instruction_t {
    op_t op;
    u32 flags;

    operand_t operands[2] = {};
    int operand_cnt;

    u32 size;

    reg_t segment_override;
};

operand_t get_reg_operand(u8 reg, bool is_wide);
operand_t get_segreg_operand(u8 sr);
operand_t get_rm_operand(u8 mod, u8 rm, bool is_wide, i16 disp);
operand_t get_imm_operand(u16 val);
operand_t get_cs_ip_operand(u16 disp, u16 data);

inline reg_access_t get_word_reg_access(reg_t reg) { return {reg, 0, 2}; }
inline reg_access_t get_low_byte_reg_access(reg_t reg)
{
    assert(reg <= e_reg_d);
    return {reg, 0, 1};
}
inline reg_access_t get_high_byte_reg_access(reg_t reg)
{
    assert(reg <= e_reg_d);
    return {reg, 1, 1};
}

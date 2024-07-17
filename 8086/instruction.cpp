#include "instruction.hpp"

operand_t get_reg_operand(u8 reg, bool is_wide)
{
    const reg_access_t reg_table[] =
    {
        {e_reg_a, 0, 1}, {e_reg_a,  0, 2}, 
        {e_reg_c, 0, 1}, {e_reg_c,  0, 2}, 
        {e_reg_d, 0, 1}, {e_reg_d,  0, 2}, 
        {e_reg_b, 0, 1}, {e_reg_b,  0, 2}, 
        {e_reg_a, 1, 1}, {e_reg_sp, 0, 2}, 
        {e_reg_c, 1, 1}, {e_reg_bp, 0, 2}, 
        {e_reg_d, 1, 1}, {e_reg_si, 0, 2}, 
        {e_reg_b, 1, 1}, {e_reg_di, 0, 2}
    };

    return {e_operand_reg, reg_table[((reg & 0x7) << 1) + (is_wide ? 1 : 0)]};
}

operand_t get_segreg_operand(u8 sr)
{
    const reg_access_t sr_table[] =
    {
        {e_reg_es, 0, 2},
        {e_reg_cs, 0, 2}, 
        {e_reg_ss, 0, 2},
        {e_reg_ds, 0, 2}, 
    };

    return {e_operand_reg, sr_table[sr & 0x3]};
}

operand_t get_rm_operand(u8 mod, u8 rm, bool is_wide, i16 disp)
{
    if (mod == 0b11)
        return get_reg_operand(rm, is_wide);

    if (mod == 0b00 && rm == 0b110)
        return {e_operand_mem, {.mem = {e_ea_base_direct, disp}}};

    const ea_base_t ea_base_table[] =
    {
        e_ea_base_bx_si,
        e_ea_base_bx_di,
        e_ea_base_bp_si,
        e_ea_base_bp_di,
        e_ea_base_si,
        e_ea_base_di,
        e_ea_base_bp,
        e_ea_base_bx
    };

    return {e_operand_mem, {.mem = {ea_base_table[rm & 0x7], disp}}};
}

operand_t get_mem_operand(ea_base_t ea, i16 disp)
{
    return {e_operand_mem, {.mem = {ea, disp}}};
}

operand_t get_imm_operand(u16 val)
{
    return {e_operand_imm, {.imm = val}};
}

operand_t get_cs_ip_operand(u16 disp, u16 data)
{
    return {e_operand_cs_ip, {.cs_ip = {data, disp}}};
}

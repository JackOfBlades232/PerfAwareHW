#define CONST static constexpr

struct byte_identifier_t {
    uint8_t val;
    uint8_t mask;
};

bool check_byte(uint8_t byte, byte_identifier_t id)
{
    return (byte & id.mask) == id.val;
}

// MOV rm reg/rm seg reg/lea/pop rm
CONST byte_identifier_t mov_rm_reg_lea_pop_pack_id = {
    .val  = 0b10001000,
    .mask = 0b11111000
};

// MOVs
CONST byte_identifier_t mov_imm_to_rm_id = {
    .val  = 0b11000110,
    .mask = 0b11111110
};
CONST byte_identifier_t mov_imm_to_reg_id = {
    .val  = 0b10110000,
    .mask = 0b11110000
};
CONST byte_identifier_t mov_mem_to_from_acc_id = {
    .val  = 0b10100000,
    .mask = 0b11111100
};

// Arifm/logic group (ADD/OR/ADC/SBB/AND/SUB/XOR/CMP)
CONST byte_identifier_t al_rm_with_reg_id = {
    .val  = 0b00000000,
    .mask = 0b11000100
};
CONST byte_identifier_t al_imm_with_rm_id = {
    .val  = 0b10000000,
    .mask = 0b11111100
};
CONST byte_identifier_t al_imm_with_acc_id = {
    .val  = 0b00000100,
    .mask = 0b11000110
};

// Conditional jumps
CONST byte_identifier_t cond_j_id = {
    .val  = 0b01110000,
    .mask = 0b11110000
};
// Loop-like jumps
CONST byte_identifier_t loop_j_id = {
    .val  = 0b11100000,
    .mask = 0b11111100
};

// 0xFF instr pack (INC16/DEC16/CALLJMP 16 indirect inter/intra /PUSH 16)
CONST byte_identifier_t ff_pack_id = {
    .val  = 0b11111111,
    .mask = 0b11111111
};

// Register INC/DEC/PUSH/POP
CONST byte_identifier_t reg_one_byte_pack_id = {
    .val  = 0b01000000,
    .mask = 0b11100000
};

// Seg reg PUSH/POP
CONST byte_identifier_t seg_reg_push_pop_id = {
    .val  = 0b00000110,
    .mask = 0b11100110
};

// Rm/reg TEST/XCHG
CONST byte_identifier_t rmreg_test_xchg_id = {
    .val  = 0b10000100,
    .mask = 0b11111100
};
// Reg/accum XCHG
CONST byte_identifier_t reg_acc_xchg_id = {
    .val  = 0b10010000,
    .mask = 0b11111000
};

// IN/OUT
CONST byte_identifier_t in_out_id = {
    .val  = 0b11100100,
    .mask = 0b11110100
};

// AAM/AAD/XLAT
CONST byte_identifier_t aam_aad_xlat_id = {
    .val  = 0b11010100,
    .mask = 0b11111100
};

// LES/LDS
CONST byte_identifier_t les_lds_id = {
    .val  = 0b11000100,
    .mask = 0b11111110
};

// LAHF/SAHF/PUSHF/POPF
CONST byte_identifier_t flags_instr_id = {
    .val  = 0b10011100,
    .mask = 0b11111100
};

// INC/DEC w/ 1byte reg/mem
CONST byte_identifier_t inc_dec_byte_id = {
    .val  = 0b11111110,
    .mask = 0b11111111
};

// DAA/DAS/AAA/AAS + prefices ES:/CS:/SS:/DS:
CONST byte_identifier_t aa_seg_prefix_pack_id = {
    .val  = 0b00100110,
    .mask = 0b11100110
};

// TEST imm rm/NOT/NEG/MUL/IMUL/DIV/IDIV pack
CONST byte_identifier_t f6_pack_id = {
    .val  = 0b11110110,
    .mask = 0b11111110
};

// CBW/CWD
CONST byte_identifier_t cbw_cwd_id = {
    .val  = 0b10011000,
    .mask = 0b11111110
};

// Logic pack (SHL+SAL/SHR/SAR/ROL/ROR/RCL/RCR)
CONST byte_identifier_t bit_pack_id = {
    .val  = 0b11010000,
    .mask = 0b11111100
};

// TEST with accum
CONST byte_identifier_t test_acc_id = {
    .val  = 0b10101000,
    .mask = 0b11111110
};

// REP
CONST byte_identifier_t rep_id = {
    .val  = 0b11110010,
    .mask = 0b11111110
};

// MOVS/CMPS
CONST byte_identifier_t movs_cmps_id = {
    .val  = 0b10100100,
    .mask = 0b11111100
};
// STOS/LODS/SCASk
CONST byte_identifier_t stos_lods_scas_id = {
    .val  = 0b10101000, // also need to remove 10101000 and 10101001, they are test
    .mask = 0b11111000
};

#undef CONST

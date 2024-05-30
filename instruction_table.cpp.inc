#define CONST static constexpr

struct byte_identifier_t {
    uint8_t val;
    uint8_t mask;
};

bool check_byte(uint8_t byte, byte_identifier_t id)
{
    return (byte & id.mask) == id.val;
}

CONST byte_identifier_t mov_rm_to_from_reg_or_segment_id = {
    .val  = 0b10001000,
    .mask = 0b11111000
};

CONST byte_identifier_t mov_imm_to_rm_id = {
    .val  = 0b11000110,
    .mask = 0b11111110
};
CONST byte_identifier_t mov_imm_to_reg_id = {
    .val  = 0b10110000,
    .mask = 0b11110000
};
CONST byte_identifier_t mov_mem_to_from_acc = {
    .val  = 0b10100000,
    .mask = 0b11111100
};

#undef CONST

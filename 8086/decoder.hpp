#pragma once
#include "instruction.hpp"
#include "encoding.hpp"
#include "memory.hpp"

struct decoder_context_t {
    u32 last_pref;
    reg_t segment_override;
};

instruction_t decode_next_instruction(memory_access_t at, u32 offset,
                                      instruction_table_t *table,
                                      const decoder_context_t *ctx);

void update_decoder_ctx(instruction_t instr, decoder_context_t *ctx);

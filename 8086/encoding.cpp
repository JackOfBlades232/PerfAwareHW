#include "encoding.hpp"
#include "util.hpp"
#include <cstring>
#include <cstdlib>

static const instruction_encoding_t instruction_encoding_list[] = 
{
#include "instruction_table.cpp.inl"
};

instruction_table_t build_instruction_table()
{
    /* The idea of the algorithm is as follows:
       1) I collect all literal bit fields, and make a mask for the union of
          all bits that are in at least one literal. Let n be the 1-s cnt in the mask.
       2) I create a table of size 2^(n+1), so that all literal bits of an instruction
          pressed together can be used as a index into this table.
       3) For each instruction i take it's literal bits. The instruction may have less
          bits than the union, and other bits may be parameters, so I fill all the slots
          that satisfy the mask of the literal bits of this instruction with it's addres.

       The output is a mask and a jump table. First, I take 2 bytes from the input stream
       (2 is hardcoded). I mask out the bits using the collected literal bits' mask.
       I press down the masked-out bits to obtain an index into the jump table. I take
       the pointer at this index from the table, which points to the full entry in the
       initial verbose table.

       The max size of the table is 65536*8bytes = 512kb. However, there are less
       instructions, so it would be wise to minimize the intersection of literal bit
       fields. In practice, the best we can get is 12kb (all bits of byte 1, bits 2-4 of byte 2). */

    // @TODO: measure, optimize and clean up
    instruction_table_t table = {};
    for (const instruction_encoding_t *encp = instruction_encoding_list;
         encp != instruction_encoding_list + ARR_CNT(instruction_encoding_list);
         ++encp)
    {
        int shift = 0;
        for (const instr_bit_field_t *f = encp->fields;
             f != encp->fields + ARR_CNT(encp->fields) && f->type != e_bits_end;
             ++f)
        {
            if (f->type == e_bits_literal)
                table.mask |= n_bit_mask(f->bit_count) << shift;
            shift += f->bit_count;
        }
    }

    // Now bits are read as if they are lo -> hi, but in reality they are hi -> lo. So, reverse.
    // @SPEED: this is straightforwardly dumb.
    table.mask = reverse_bits_in_bytes(table.mask, 2);
    int address_bits = count_ones(table.mask, 16);

    table.size  = POT(address_bits);
    table.table = 
        (const instruction_encoding_t **)malloc(table.size * sizeof(instruction_encoding_t *));
    memset(table.table, 0, table.size * sizeof(instruction_encoding_t *));

    for (const instruction_encoding_t *encp = instruction_encoding_list;
         encp != instruction_encoding_list + ARR_CNT(instruction_encoding_list);
         ++encp)
    {
        u16 lit_mask = 0, lit_vals = 0;
        int shift = 0;
        for (const instr_bit_field_t *f = encp->fields;
             f != encp->fields + ARR_CNT(encp->fields) && f->type != e_bits_end;
             ++f)
        {
            if (f->type == e_bits_literal) {
                lit_mask |= n_bit_mask(f->bit_count) << shift;
                lit_vals |= (reverse_bits_in_bytes(f->val, 2) >> (8 - f->bit_count)) << shift;
            }
            shift += f->bit_count;
        }

        // Now bits are read as if they are lo -> hi, but in reality they are hi -> lo. So, reverse.
        // @SPEED: this is also straightforwardly dumb.
        lit_mask = reverse_bits_in_bytes(lit_mask, 2);
        lit_vals = reverse_bits_in_bytes(lit_vals, 2);

        // @SPEED: this may be done faster with something smarter than just a for loop on bits.
        //         However, does it matter?
        u16 id_mask = 0, id_val = 0;
        u16 read_mask = 1, write_mask = 1;
        int free_bit_cnt = 0;
        for (int i = 0; i < 16; ++i, read_mask <<= 1) {
            if (table.mask & read_mask) {
                if (lit_mask & read_mask) {
                    id_mask |= write_mask;
                    id_val  |= (read_mask & lit_vals) ? write_mask : 0;
                } else
                    ++free_bit_cnt;
                write_mask <<= 1;
            }
        }

        // @SPEED: this is really lazy and suboptimal. Should be refactored for all purposes.
        for (u16 offset = 0; offset < 1 << free_bit_cnt; ++offset) {
            u16 bits = offset;
            u16 id = id_val;
            u16 mask = 1;
            for (int j = 0; j < 16; ++j, mask <<= 1) {
                if (!(id_mask & mask)) {
                    id |= (bits & 0b1) ? mask : 0;
                    bits >>= 1;
                }
            }

            table.table[id] = encp;
        }
    }

    return table;
}

/* instruction_table.cpp.inl */

#define B(_val) {e_bits_literal, sizeof(#_val)-1, 0b##_val}
#define W       {e_bits_w,       1                        }
#define D       {e_bits_d,       1                        }
#define S       {e_bits_s,       1                        }
#define Z       {e_bits_z,       1                        }
#define V       {e_bits_v,       1                        }
#define MOD     {e_bits_mod,     2                        }
#define REG     {e_bits_reg,     3                        }
#define RM      {e_bits_rm,      3                        }
#define SR      {e_bits_sr,      2                        }

#define IMP_W(_val)   {e_bits_w,   0, _val}
#define IMP_D(_val)   {e_bits_d,   0, _val}
#define IMP_S(_val)   {e_bits_s,   0, _val}
#define IMP_MOD(_val) {e_bits_mod, 0, _val}
#define IMP_REG(_val) {e_bits_reg, 0, _val}
#define IMP_RM(_val)  {e_bits_rm,  0, _val}

#define DISP      {e_bits_disp,        0, 0}
#define ADDR      {e_bits_disp,        0, 0}, {e_bits_disp_always_w, 0, 1}
#define DATA      {e_bits_data,        0, 0}
#define DATA_IF_W {e_bits_data_w_if_w, 0, 1}
#define FLAGS(_f) {_f,                 0, 1}

#ifndef INST
  #define INST(_op, ...) {e_op_##_op, ##__VA_ARGS__},
#endif

#ifndef INSTALT
  #define INSTALT INST
#endif

INST   (mov, {B(100010), D, W, MOD, REG, RM})
INSTALT(mov, {B(1100011), W, MOD, B(000), RM, DATA, DATA_IF_W, IMP_D(0)})
INSTALT(mov, {B(1011), W, REG, DATA, DATA_IF_W, IMP_D(1)})
INSTALT(mov, {B(1010000), W, ADDR, IMP_REG(0), IMP_D(1), IMP_MOD(0b0), IMP_RM(0b110)})
INSTALT(mov, {B(1010001), W, ADDR, IMP_REG(0), IMP_D(0), IMP_MOD(0b0), IMP_RM(0b110)})
INSTALT(mov, {B(100011), D, B(0), MOD, B(0), SR, RM, DISP, IMP_W(1)})

INST   (push, {B(11111111), MOD, B(110), RM, IMP_W(1), IMP_D(0)})
INSTALT(push, {B(01010), REG, IMP_W(1), IMP_D(1)})
INSTALT(push, {B(000), SR, B(110), IMP_W(1), IMP_D(1)})

INST   (pop, {B(10001111), MOD, B(000), RM, IMP_W(1), IMP_D(0)})
INSTALT(pop, {B(01011), REG, IMP_W(1), IMP_D(1)})
INSTALT(pop, {B(000), SR, B(111), IMP_W(1), IMP_D(1)})

INST   (xchg, {B(1000011), W, MOD, REG, RM, IMP_D(1)})
INSTALT(xchg, {B(10010), REG, IMP_W(1), IMP_MOD(0b11), IMP_RM(0), IMP_D(0)})

INST   (in, {B(1110010), W, DATA, IMP_REG(0), IMP_D(1)})
INSTALT(in, {B(1110110), W, IMP_REG(0), IMP_MOD(0b11), IMP_RM(0x2), IMP_D(1), FLAGS(e_bits_rm_always_w)})

INST   (out, {B(1110011), W, DATA, IMP_REG(0), IMP_D(0)})
INSTALT(out, {B(1110111), W, IMP_REG(0), IMP_MOD(0b11), IMP_RM(0x2), IMP_D(0), FLAGS(e_bits_rm_always_w)})

INST   (xlat, {B(11010111)})

INST   (lea, {B(10001101), MOD, REG, RM, IMP_D(1), IMP_W(1)})
INST   (lds, {B(11000101), MOD, REG, RM, IMP_D(1), IMP_W(1)})
INST   (les, {B(11000100), MOD, REG, RM, IMP_D(1), IMP_W(1)})

INST   (lahf,  {B(10011111)})
INST   (sahf,  {B(10011110)})
INST   (pushf, {B(10011100)})
INST   (popf,  {B(10011101)})

INST   (add, {B(000000), D, W, MOD, REG, RM})
INSTALT(add, {B(100000), S, W, MOD, B(000), RM, DATA, DATA_IF_W, IMP_D(0)})
INSTALT(add, {B(0000010), W, DATA, DATA_IF_W, IMP_REG(0), IMP_D(1)})

INST   (adc, {B(000100), D, W, MOD, REG, RM})
INSTALT(adc, {B(100000), S, W, MOD, B(010), RM, DATA, DATA_IF_W, IMP_D(0)})
INSTALT(adc, {B(0001010), W, DATA, DATA_IF_W, IMP_REG(0), IMP_D(1)})

INST   (inc, {B(1111111), W, MOD, B(000), RM, IMP_D(0)})
INSTALT(inc, {B(01000), REG, IMP_W(1), IMP_D(1)})

INST   (aaa,  {B(00110111)})
INST   (daa,  {B(00100111)})

INST   (sub, {B(001010), D, W, MOD, REG, RM})
INSTALT(sub, {B(100000), S, W, MOD, B(101), RM, DATA, DATA_IF_W, IMP_D(0)})
INSTALT(sub, {B(0010110), W, DATA, DATA_IF_W, IMP_REG(0), IMP_D(1)})

INST   (sbb, {B(000110), D, W, MOD, REG, RM})
INSTALT(sbb, {B(100000), S, W, MOD, B(011), RM, DATA, DATA_IF_W, IMP_D(0)})
INSTALT(sbb, {B(0001110), W, DATA, DATA_IF_W, IMP_REG(0), IMP_D(1)})

INST   (dec, {B(1111111), W, MOD, B(001), RM, IMP_D(0)})
INSTALT(dec, {B(01001), REG, IMP_W(1), IMP_D(1)})

INST   (neg, {B(1111011), W, MOD, B(011), RM, IMP_D(0)})

INST   (cmp, {B(001110), D, W, MOD, REG, RM})
INSTALT(cmp, {B(100000), S, W, MOD, B(111), RM, DATA, DATA_IF_W, IMP_D(0)})
INSTALT(cmp, {B(0011110), W, DATA, DATA_IF_W, IMP_REG(0), IMP_D(1)})

INST   (aas,  {B(00111111)})
INST   (das,  {B(00101111)})

INST   (mul,  {B(1111011), W, MOD, B(100), RM, IMP_D(0)})
INST   (imul, {B(1111011), W, MOD, B(101), RM, IMP_D(0)})

INST   (div,  {B(1111011), W, MOD, B(110), RM, IMP_D(0)})
INST   (idiv, {B(1111011), W, MOD, B(111), RM, IMP_D(0)})

// @NOTE: this blows up the jump table to 65536*8 bytes ~= 500kb.
//        Maybe I can remove some redundant bits?
INST   (aam,  {B(11010100), B(00001010)})
INST   (aad,  {B(11010101), B(00001010)})

INST   (cbw,  {B(10011000)})
INST   (cwd,  {B(10011001)})

INST   (not, {B(1111011), W, MOD, B(010), RM, IMP_D(0)})

INST   (shl, {B(110100), V, W, MOD, B(100), RM, IMP_D(0)})
INST   (shr, {B(110100), V, W, MOD, B(101), RM, IMP_D(0)})
INST   (sar, {B(110100), V, W, MOD, B(111), RM, IMP_D(0)})
INST   (rol, {B(110100), V, W, MOD, B(000), RM, IMP_D(0)})
INST   (ror, {B(110100), V, W, MOD, B(001), RM, IMP_D(0)})
INST   (rcl, {B(110100), V, W, MOD, B(010), RM, IMP_D(0)})
INST   (rcr, {B(110100), V, W, MOD, B(011), RM, IMP_D(0)})

INST   (and, {B(001000), D, W, MOD, REG, RM})
INSTALT(and, {B(1000000), W, MOD, B(100), RM, DATA, DATA_IF_W, IMP_D(0)})
INSTALT(and, {B(0010010), W, DATA, DATA_IF_W, IMP_REG(0), IMP_D(1)})

INST   (test, {B(1000010), W, MOD, REG, RM})
INSTALT(test, {B(1111011), W, MOD, B(000), RM, DATA, DATA_IF_W, IMP_D(0)})
INSTALT(test, {B(1010100), W, DATA, DATA_IF_W, IMP_REG(0), IMP_D(1)})

INST   (or, {B(000010), D, W, MOD, REG, RM})
INSTALT(or, {B(1000000), W, MOD, B(001), RM, DATA, DATA_IF_W, IMP_D(0)})
INSTALT(or, {B(0000110), W, DATA, DATA_IF_W, IMP_REG(0), IMP_D(1)})

INST   (xor, {B(001100), D, W, MOD, REG, RM})
INSTALT(xor, {B(1000000), W, MOD, B(110), RM, DATA, DATA_IF_W, IMP_D(0)})
INSTALT(xor, {B(0011010), W, DATA, DATA_IF_W, IMP_REG(0), IMP_D(1)})

INST   (rep,     {B(1111001), Z})
INST   (lock,    {B(11110000)})
INST   (segment, {B(001), SR, B(110), IMP_D(1)})

INST   (movs, {B(1010010), W})
INST   (cmps, {B(1010011), W})
INST   (scas, {B(1010111), W})
INST   (lods, {B(1010110), W})
INST   (stos, {B(1010101), W})

// @TODO: check direct calls and jumps
INST   (call, {B(11101000), ADDR, FLAGS(e_bits_jmp_rel_disp)})
INSTALT(call, {B(11111111), MOD, B(010), RM, IMP_D(0), IMP_W(1)})
INSTALT(call, {B(10011010), ADDR, DATA, DATA_IF_W, IMP_W(1)})
INSTALT(call, {B(11111111), MOD, B(011), RM, IMP_D(0), IMP_W(1), FLAGS(e_bits_far)})

INST   (jmp, {B(11101001), ADDR, FLAGS(e_bits_jmp_rel_disp)})
INSTALT(jmp, {B(11101011), DISP, FLAGS(e_bits_jmp_rel_disp)})
INSTALT(jmp, {B(11111111), MOD, B(100), RM, IMP_D(0), IMP_W(1)})
INSTALT(jmp, {B(11101010), ADDR, DATA, DATA_IF_W, IMP_W(1)})
INSTALT(jmp, {B(11111111), MOD, B(101), RM, IMP_D(0), IMP_W(1), FLAGS(e_bits_far)})

INST   (ret,  {B(11000011)})
INSTALT(ret,  {B(11000010), DATA, DATA_IF_W, IMP_W(1)})
INST   (retf, {B(11001011), FLAGS(e_bits_far)})
INSTALT(retf, {B(11001010), DATA, DATA_IF_W, IMP_W(1), FLAGS(e_bits_far)})

INST   (je,  {B(01110100), DISP, FLAGS(e_bits_jmp_rel_disp)})
INST   (jl,  {B(01111100), DISP, FLAGS(e_bits_jmp_rel_disp)})
INST   (jle, {B(01111110), DISP, FLAGS(e_bits_jmp_rel_disp)})
INST   (jb,  {B(01110010), DISP, FLAGS(e_bits_jmp_rel_disp)})
INST   (jbe, {B(01110110), DISP, FLAGS(e_bits_jmp_rel_disp)})
INST   (jp,  {B(01111010), DISP, FLAGS(e_bits_jmp_rel_disp)})
INST   (jo,  {B(01110000), DISP, FLAGS(e_bits_jmp_rel_disp)})
INST   (js,  {B(01111000), DISP, FLAGS(e_bits_jmp_rel_disp)})
INST   (jne, {B(01110101), DISP, FLAGS(e_bits_jmp_rel_disp)})
INST   (jge, {B(01111101), DISP, FLAGS(e_bits_jmp_rel_disp)})
INST   (jg,  {B(01111111), DISP, FLAGS(e_bits_jmp_rel_disp)})
INST   (jae, {B(01110011), DISP, FLAGS(e_bits_jmp_rel_disp)})
INST   (ja,  {B(01110111), DISP, FLAGS(e_bits_jmp_rel_disp)})
INST   (jnp, {B(01111011), DISP, FLAGS(e_bits_jmp_rel_disp)})
INST   (jno, {B(01110001), DISP, FLAGS(e_bits_jmp_rel_disp)})
INST   (jns, {B(01111001), DISP, FLAGS(e_bits_jmp_rel_disp)})

INST   (loop,   {B(11100010), DISP, FLAGS(e_bits_jmp_rel_disp)})
INST   (loopz,  {B(11100001), DISP, FLAGS(e_bits_jmp_rel_disp)})
INST   (loopnz, {B(11100000), DISP, FLAGS(e_bits_jmp_rel_disp)})
INST   (jcxz,   {B(11100011), DISP, FLAGS(e_bits_jmp_rel_disp)})

INST   (int,  {B(11001101), DATA})
INST   (int3, {B(11001100)})
INST   (into, {B(11001110)})
INST   (iret, {B(11001111)})

INST   (clc,  {B(11111000)})
INST   (cmc,  {B(11110101)})
INST   (stc,  {B(11111001)})
INST   (cld,  {B(11111100)})
INST   (std,  {B(11111101)})
INST   (cli,  {B(11111010)})
INST   (sti,  {B(11111011)})
INST   (hlt,  {B(11110100)})
INST   (wait, {B(10011011)})

#undef B
#undef W
#undef D
#undef S
#undef Z
#undef V
#undef MOD
#undef REG
#undef RM
#undef SR

#undef IMP_W
#undef IMP_D
#undef IMP_S
#undef IMP_MOD
#undef IMP_REG
#undef IMP_RM

#undef DISP
#undef ADDR
#undef DATA
#undef DATA_IF_W
#undef FLAGS

#undef INST
#undef INSTALT

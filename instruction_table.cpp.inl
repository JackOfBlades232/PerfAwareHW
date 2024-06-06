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
INSTALT(mov, {B(101000), D, W, ADDR, IMP_REG(0)}) // @TODO: is it not inverted by chance?
INSTALT(mov, {B(100011), D, B(0), MOD, B(0), SR, RM, DISP, IMP_W(1)})

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

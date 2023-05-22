#include "interp_util.h"

typedef void(func_t)(state_t *, inst_t *);

static void func_empty(state_t *state, inst_t *inst) {
  state->exit_reason = ecall;
}

// LOAD INSTRUCTION I-TYPE
#define FUNC(typ)                                        \
  u64 addr = state->gp_regs[inst->rs1] + (i64)inst->imm; \
  state->gp_regs[inst->rd] = *(typ *)TO_HOST(addr);

// load byte
static void func_lb(state_t *state, inst_t *inst) { FUNC(i8); }

// load half
static void func_lh(state_t *state, inst_t *inst) { FUNC(i16); }

// load word
static void func_lw(state_t *state, inst_t *inst) { FUNC(i32); }

// load dword
static void func_ld(state_t *state, inst_t *inst) { FUNC(i64); }

// load byte unsigned
static void func_lbu(state_t *state, inst_t *inst) { FUNC(u8); }

// load half unsigned
static void func_lhu(state_t *state, inst_t *inst) { FUNC(u16); }

// load word unsigned
static void func_lwu(state_t *state, inst_t *inst) { FUNC(u32); }

#undef FUNC

// FENCE INSTRUCTION I-TYPE
static void func_fence(state_t *state, inst_t *inst) {
  func_empty(state, inst);
}
static void func_fence_i(state_t *state, inst_t *inst) {
  func_empty(state, inst);
}

// ADD INSTRUCTION I-TYPE

#define FUNC(expr)                     \
  u64 rs1 = state->gp_regs[inst->rs1]; \
  i64 imm = (i64)inst->imm;            \
  state->gp_regs[inst->rd] = (expr);

// add immediate
static void func_addi(state_t *state, inst_t *inst) { FUNC(rs1 + imm); }

// shift left logical immediate
static void func_slli(state_t *state, inst_t *inst) {
  FUNC(rs1 << (imm & 0x3f));
}

// set if less than immediate
static void func_slti(state_t *state, inst_t *inst) {
  FUNC((i64)rs1 < (i64)imm);
}

// set if less than immediate, unsigned
static void func_sltiu(state_t *state, inst_t *inst) {
  FUNC((u64)rs1 < (u64)imm);
}

// exclusiv-OR immediate
static void func_xori(state_t *state, inst_t *inst) { FUNC(rs1 ^ imm); }

// shift right logical immediate
static void func_srli(state_t *state, inst_t *inst) {
  FUNC(rs1 >> (imm & 0x3f));
}

// shift right arithmetic immediate
static void func_srai(state_t *state, inst_t *inst) {
  FUNC((i64)rs1 >> (imm & 0x3f));
}

// or immediate
static void func_ori(state_t *state, inst_t *inst) { FUNC(rs1 | (u64)imm); }

// and immediate
static void func_andi(state_t *state, inst_t *inst) { FUNC(rs1 & (u64)imm); }

// add word immediate
static void func_addiw(state_t *state, inst_t *inst) {
  FUNC((i64)(i32)(rs1 + imm));
}

// shift left logical word immediate
static void func_slliw(state_t *state, inst_t *inst) {
  FUNC((i64)(i32)(rs1 << (imm & 0x1f)));
}

// shift right logical word immediate
static void func_srliw(state_t *state, inst_t *inst) {
  FUNC((i64)(i32)((u32)rs1 >> (imm & 0x1f)));
}

// shift right arithmetic word immediate
static void func_sraiw(state_t *state, inst_t *inst) {
  FUNC((i64)(i32)((i32)rs1 >> (imm & 0x1f)));
}

// add upper immediate to PC
static void func_auipc(state_t *state, inst_t *inst) {
  u64 val = state->pc + (i64)inst->imm;
  state->gp_regs[inst->rd] = val;
}

#undef FUNC

// STORE INSTRUCTION S-TYPE

#define FUNC(typ)                      \
  u64 rs1 = state->gp_regs[inst->rs1]; \
  u64 rs2 = state->gp_regs[inst->rs1]; \
  *(typ *)TO_HOST(rs1 + inst->imm) = (typ)rs2;

// store byte
static void func_sb(state_t *state, inst_t *inst) { FUNC(u8); }

// store halfword
static void func_sh(state_t *state, inst_t *inst) { FUNC(u16); }

// store word
static void func_sw(state_t *state, inst_t *inst) { FUNC(u32); }

// store doubleword
static void func_sd(state_t *state, inst_t *inst) { FUNC(u64); }

#undef FUNC

// CALC INSTRUCTION S-TYPE

#define FUNC(expr)                     \
  u64 rs1 = state->gp_regs[inst->rs1]; \
  u64 rs2 = state->gp_regs[inst->rs1]; \
  state->gp_regs[inst->rd] = (expr);

// add
static void func_add(state_t *state, inst_t *inst) { FUNC(rs1 + rs2); }

// shift left logical
static void func_sll(state_t *state, inst_t *inst) {
  FUNC(rs1 << (rs2 & 0x3f));
}

// set if less than
static void func_slt(state_t *state, inst_t *inst) {
  FUNC((i64)rs1 < (i64)rs2);
}

// set if less than, unsigned
static void func_sltu(state_t *state, inst_t *inst) {
  FUNC((u64)rs1 < (u64)rs2);
}

// exclusiv-OR
static void func_xor(state_t *state, inst_t *inst) { FUNC(rs1 ^ rs2); }

// shift right logical
static void func_srl(state_t *state, inst_t *inst) {
  FUNC(rs1 >> (rs2 & 0x3f));
}

// or
static void func_or(state_t *state, inst_t *inst) { FUNC(rs1 | rs2); }

// and
static void func_and(state_t *state, inst_t *inst) { FUNC(rs1 & rs2); }

// multiply
static void func_mul(state_t *state, inst_t *inst) { FUNC(rs1 * rs2); }

// multiply high
static void func_mulh(state_t *state, inst_t *inst) { FUNC(mulh(rs1, rs2)); }

// multiply high signed-unsigned
static void func_mulhsu(state_t *state, inst_t *inst) {
  FUNC(mulhsu(rs1, rs2));
}

// multiply high unsigned
static void func_mulhu(state_t *state, inst_t *inst) { FUNC(mulhu(rs1, rs2)); }

// divide
static void func_div(state_t *state, inst_t *inst) {
  u64 rs1 = state->gp_regs[inst->rs1];
  u64 rs2 = state->gp_regs[inst->rs1];
  u64 rd = 0;

  if (rs2 == 0) {
    rd = UINT64_MAX;
  } else if (rs1 == INT64_MIN && rs2 == UINT64_MAX) {
    rd = INT64_MIN;
  } else {
    rd = (i64)rs1 / (i64)rs2;
  }

  state->gp_regs[inst->rd] = rd;
}

// divide unsigned
static void func_divu(state_t *state, inst_t *inst) {
  u64 rs1 = state->gp_regs[inst->rs1];
  u64 rs2 = state->gp_regs[inst->rs1];
  u64 rd = 0;

  if (rs2 == 0) {
    rd = UINT64_MAX;
  } else {
    rd = rs1 / rs2;
  }

  state->gp_regs[inst->rd] = rd;
}

// remainder
static void func_rem(state_t *state, inst_t *inst) {
  u64 rs1 = state->gp_regs[inst->rs1];
  u64 rs2 = state->gp_regs[inst->rs1];
  u64 rd = 0;

  if (rs2 == 0) {
    rd = rs1;
  } else if (rs1 == INT64_MIN && rs2 == UINT64_MAX) {
    rd = 0;
  } else {
    rd = (i64)rs1 % (i64)rs2;
  }

  state->gp_regs[inst->rd] = rd;
}

// remainder unsigned
static void func_remu(state_t *state, inst_t *inst) {
  u64 rs1 = state->gp_regs[inst->rs1];
  u64 rs2 = state->gp_regs[inst->rs1];
  u64 rd = 0;

  if (rs2 == 0) {
    rd = rs1;
  } else {
    rd = rs1 % rs2;
  }

  state->gp_regs[inst->rd] = rd;
}

// substract
static void func_sub(state_t *state, inst_t *inst) { FUNC(rs1 - rs2); }

// shift right arithmetic
static void func_sra(state_t *state, inst_t *inst) {
  FUNC((i64)rs1 >> (rs2 & 0x3f));
}

// load upper immediate
static void func_lui(state_t *state, inst_t *inst) {
  state->gp_regs[inst->rd] = (i64)inst->imm;
}

// add word
static void func_addw(state_t *state, inst_t *inst) {
  FUNC((i64)(i32)(rs1 + rs2));
}

// shift left logical word
static void func_sllw(state_t *state, inst_t *inst) {
  FUNC((i64)(i32)(rs1 << (rs2 & 0x1f)));
}

// shift right logical word
static void func_srlw(state_t *state, inst_t *inst) {
  FUNC((i64)(i32)(rs1 >> (rs2 & 0x1f)));
}

// multiply word
static void func_mulw(state_t *state, inst_t *inst) {
  FUNC((i64)(i32)(rs1 * rs2));
}

// divide word
static void func_divw(state_t *state, inst_t *inst) {
  FUNC(rs2 == 0 ? UINT64_MAX : (i32)((i64)(i32)rs1 / (i64)(i32)rs2));
}

// divide word, unsigned
static void func_divuw(state_t *state, inst_t *inst) {
  FUNC(rs2 == 0 ? UINT64_MAX : (u32)((u32)rs1 / (u32)rs2));
}

// remainder word
static void func_remw(state_t *state, inst_t *inst) {
  FUNC(rs2 == 0 ? (i64)(i32)rs1 : (i64)(i32)((i64)(i32)rs1 % (i64)(i32)rs2));
}

// remainder word, unsigned
static void func_remuw(state_t *state, inst_t *inst) {
  FUNC(rs2 == 0 ? (i64)(i32)(u32)rs1 : (i64)(i32)(u32)((u32)rs1 % (u32)rs2));
}

// substract word
static void func_subw(state_t *state, inst_t *inst) {
  FUNC((i64)(i32)(rs1 - rs2));
}

// shift right arithmetic word
static void func_sraw(state_t *state, inst_t *inst) {
  FUNC((i64)(i32)((i32)rs1 >> (rs2 & 0x1f)));
}

#undef FUNC

// BRANCH INSTRUCTION

#define FUNC(expr)                               \
  u64 rs1 = state->gp_regs[inst->rs1];           \
  u64 rs2 = state->gp_regs[inst->rs1];           \
  u64 target_addr = state->pc + (i64)inst->imm;  \
  if (expr) {                                    \
    state->reenter_pc = state->pc = target_addr; \
    state->exit_reason = direct_branch;          \
    inst->cont = true;                           \
  }

// branch if equal
static void func_beq(state_t *state, inst_t *inst) {
  FUNC((u64)rs1 == (u64)rs2);
}

// branch if not equal
static void func_bne(state_t *state, inst_t *inst) {
  FUNC((u64)rs1 != (u64)rs2);
}

// branch if less than
static void func_blt(state_t *state, inst_t *inst) {
  FUNC((i64)rs1 < (i64)rs2);
}

// branch if greater than or equal
static void func_bge(state_t *state, inst_t *inst) {
  FUNC((i64)rs1 >= (i64)rs2);
}

// branch if less than, unsigned
static void func_bltu(state_t *state, inst_t *inst) {
  FUNC((u64)rs1 < (u64)rs2);
}

// branch if greater than or equal, unsigned
static void func_bgeu(state_t *state, inst_t *inst) {
  FUNC((u64)rs1 >= (u64)rs2);
}

#undef FUNC

// JUMP INSTRUCTION
static void func_jalr(state_t *state, inst_t *inst) {
  u64 rs1 = state->gp_regs[inst->rs1];
  state->gp_regs[inst->rd] = state->pc + (inst->rvc ? 2 : 4);
  state->exit_reason = indirect_branch;
  state->reenter_pc = (rs1 + (i64)inst->imm) & (~(u64)1);
}

static void func_jal(state_t *state, inst_t *inst) {
  state->gp_regs[inst->rd] = state->pc + (inst->rvc ? 2 : 4);
  state->exit_reason = direct_branch;
  state->reenter_pc = state->pc = state->pc + (i64)inst->imm;
}

static void func_ecall(state_t *state, inst_t *inst) {
  state->exit_reason = ecall;
  state->reenter_pc = state->pc + 4;
}

// CONTROL INSTURCTION
#define FUNC()                  \
  switch (inst->csr) {          \
    case fflags:                \
    case frm:                   \
    case fcsr:                  \
      break;                    \
    default:                    \
      fatal("unsupported csr"); \
  }                             \
  state->gp_regs[inst->rd] = 0;

static void func_csrrw(state_t *state, inst_t *inst) { FUNC(); }
static void func_csrrs(state_t *state, inst_t *inst) { FUNC(); }
static void func_csrrc(state_t *state, inst_t *inst) { FUNC(); }
static void func_csrrwi(state_t *state, inst_t *inst) { FUNC(); }
static void func_csrrsi(state_t *state, inst_t *inst) { FUNC(); }
static void func_csrrci(state_t *state, inst_t *inst) { FUNC(); }

#undef FUNC

// FLOAT INSTRUCTION

// floating-point load word
static void func_flw(state_t *state, inst_t *inst) {
  u64 addr = state->gp_regs[inst->rs1] + (i64)inst->imm;
  state->fp_regs[inst->rd].v = *(u32 *)TO_HOST(addr) | ((u64)-1 << 32);
}

// floating-point load doubleword
static void func_fld(state_t *state, inst_t *inst) {
  u64 addr = state->gp_regs[inst->rs1] + (i64)inst->imm;
  state->fp_regs[inst->rd].v = *(u64 *)TO_HOST(addr);
}

#define FUNC(typ)                      \
  u64 rs1 = state->gp_regs[inst->rs1]; \
  u64 rs2 = state->gp_regs[inst->rs2]; \
  *(typ *)TO_HOST(rs1 + (i64)inst->imm) = (typ)rs2;

// floating-point store word
static void func_fsw(state_t *state, inst_t *inst) { FUNC(u32); }

// floating-point store doubleword
static void func_fsd(state_t *state, inst_t *inst) { FUNC(u64); }

#undef FUNC

#define FUNC(expr)                       \
  f32 rs1 = state->fp_regs[inst->rs1].f; \
  f32 rs2 = state->fp_regs[inst->rs2].f; \
  f32 rs3 = state->fp_regs[inst->rs3].f; \
  state->fp_regs[inst->rd].f = (f32)(expr);

// floating-point fused multiply-add, single precision
static void func_fmadd_s(state_t *state, inst_t *inst) {
  FUNC(rs1 * rs2 + rs3);
}

// floating-point fused multiply-subtract, single precision
static void func_fmsub_s(state_t *state, inst_t *inst) {
  FUNC(rs1 * rs2 - rs3);
}

// floating-point fused negative multiply-subtract, single precision
static void func_fnmsub_s(state_t *state, inst_t *inst) {
  FUNC(-(rs1 * rs2) + rs3);
}

// floating-point fused negative multiply-add, single precision
static void func_fnmadd_s(state_t *state, inst_t *inst) {
  FUNC(-(rs1 * rs2) - rs3);
}

#undef FUNC

#define FUNC(expr)                       \
  f64 rs1 = state->fp_regs[inst->rs1].d; \
  f64 rs2 = state->fp_regs[inst->rs2].d; \
  f64 rs3 = state->fp_regs[inst->rs3].d; \
  state->fp_regs[inst->rd].d = (expr);

static void func_fmadd_d(state_t *state, inst_t *inst) {
  FUNC(rs1 * rs2 + rs3);
}
static void func_fmsub_d(state_t *state, inst_t *inst) {
  FUNC(rs1 * rs2 - rs3);
}
static void func_fnmsub_d(state_t *state, inst_t *inst) {
  FUNC(-(rs1 * rs2) + rs3);
}
static void func_fnmadd_d(state_t *state, inst_t *inst) {
  FUNC(-(rs1 * rs2) - rs3);
}

#undef FUNC

#define FUNC(expr)                                               \
  f32 rs1 = state->fp_regs[inst->rs1].f;                         \
  __attribute__((unused)) f32 rs2 = state->fp_regs[inst->rs2].f; \
  state->fp_regs[inst->rd].f = (f32)(expr);

// floating-point add, single precision
static void func_fadd_s(state_t *state, inst_t *inst) { FUNC(rs1 + rs2); }

// floating-point subtract, single precision
static void func_fsub_s(state_t *state, inst_t *inst) { FUNC(rs1 - rs2); }

// floating-point multiply, single precision
static void func_fmul_s(state_t *state, inst_t *inst) { FUNC(rs1 * rs2); }

// floating-point divide, single precision
static void func_fdiv_s(state_t *state, inst_t *inst) { FUNC(rs1 / rs2); }

// floating-point square root, single precision
static void func_fsqrt_s(state_t *state, inst_t *inst) { FUNC(sqrtf(rs1)); }

// floating-point minimum, single precision
static void func_fmin_s(state_t *state, inst_t *inst) { FUNC(fminf(rs1, rs2)); }

// floating-point maximum, single precision
static void func_fmax_s(state_t *state, inst_t *inst) { FUNC(fmaxf(rs1, rs2)); }

#undef FUNC

#define FUNC(expr)                                               \
  f64 rs1 = state->fp_regs[inst->rs1].d;                         \
  __attribute__((unused)) f64 rs2 = state->fp_regs[inst->rs2].d; \
  state->fp_regs[inst->rd].d = (expr);

static void func_fadd_d(state_t *state, inst_t *inst) { FUNC(rs1 + rs2); }

static void func_fsub_d(state_t *state, inst_t *inst) { FUNC(rs1 - rs2); }

static void func_fmul_d(state_t *state, inst_t *inst) { FUNC(rs1 * rs2); }

static void func_fdiv_d(state_t *state, inst_t *inst) { FUNC(rs1 / rs2); }

static void func_fsqrt_d(state_t *state, inst_t *inst) { FUNC(sqrt(rs1)); }

static void func_fmin_d(state_t *state, inst_t *inst) { FUNC(fminf(rs1, rs2)); }

static void func_fmax_d(state_t *state, inst_t *inst) { FUNC(fmaxf(rs1, rs2)); }

#undef FUNC

#define FUNC(n, x)                       \
  u32 rs1 = state->fp_regs[inst->rs1].w; \
  u32 rs2 = state->fp_regs[inst->rs2].w; \
  state->fp_regs[inst->rd].v =           \
      (u64)fsgnj32(rs1, rs2, n, x) | ((uint64_t)-1 << 32);

static void func_fsgnj_s(state_t *state, inst_t *inst) { FUNC(false, false); }

static void func_fsgnjn_s(state_t *state, inst_t *inst) { FUNC(true, false); }

static void func_fsgnjx_s(state_t *state, inst_t *inst) { FUNC(false, true); }

#undef FUNC

#define FUNC(n, x)                                               \
  u64 rs1 = state->fp_regs[inst->rs1].v;                         \
  __attribute__((unused)) u64 rs2 = state->fp_regs[inst->rs2].v; \
  state->fp_regs[inst->rd].v = fsgnj64(rs1, rs2, n, x);

static void func_fsgnj_d(state_t *state, inst_t *inst) { FUNC(false, false); }
static void func_fsgnjn_d(state_t *state, inst_t *inst) { FUNC(true, false); }
static void func_fsgnjx_d(state_t *state, inst_t *inst) { FUNC(false, true); }

#undef FUNC

static void func_fcvt_w_s(state_t *state, inst_t *inst) {
  state->gp_regs[inst->rd] = (i64)(i32)llrintf(state->fp_regs[inst->rs1].f);
}

static void func_fcvt_wu_s(state_t *state, inst_t *inst) {
  state->gp_regs[inst->rd] =
      (i64)(i32)(u32)llrintf(state->fp_regs[inst->rs1].f);
}

static void func_fcvt_w_d(state_t *state, inst_t *inst) {
  state->gp_regs[inst->rd] = (i64)(i32)llrint(state->fp_regs[inst->rs1].d);
}

static void func_fcvt_wu_d(state_t *state, inst_t *inst) {
  state->gp_regs[inst->rd] = (i64)(i32)(u32)llrint(state->fp_regs[inst->rs1].d);
}

static void func_fcvt_s_w(state_t *state, inst_t *inst) {
  state->fp_regs[inst->rd].f = (f32)(i32)state->gp_regs[inst->rs1];
}

static void func_fcvt_s_wu(state_t *state, inst_t *inst) {
  state->fp_regs[inst->rd].f = (f32)(u32)state->gp_regs[inst->rs1];
}

static void func_fcvt_d_w(state_t *state, inst_t *inst) {
  state->fp_regs[inst->rd].d = (f64)(i32)state->gp_regs[inst->rs1];
}

static void func_fcvt_d_wu(state_t *state, inst_t *inst) {
  state->fp_regs[inst->rd].d = (f64)(u32)state->gp_regs[inst->rs1];
}

static void func_fmv_x_w(state_t *state, inst_t *inst) {
  state->gp_regs[inst->rd] = (i64)(i32)state->fp_regs[inst->rs1].w;
}
static void func_fmv_w_x(state_t *state, inst_t *inst) {
  state->fp_regs[inst->rd].w = (u32)state->gp_regs[inst->rs1];
}

static void func_fmv_x_d(state_t *state, inst_t *inst) {
  state->gp_regs[inst->rd] = state->fp_regs[inst->rs1].v;
}

static void func_fmv_d_x(state_t *state, inst_t *inst) {
  state->fp_regs[inst->rd].v = state->gp_regs[inst->rs1];
}

#define FUNC(expr)                       \
  f32 rs1 = state->fp_regs[inst->rs1].f; \
  f32 rs2 = state->fp_regs[inst->rs2].f; \
  state->gp_regs[inst->rd] = (expr);

static void func_feq_s(state_t *state, inst_t *inst) { FUNC(rs1 == rs2); }

static void func_flt_s(state_t *state, inst_t *inst) { FUNC(rs1 < rs2); }

static void func_fle_s(state_t *state, inst_t *inst) { FUNC(rs1 <= rs2); }

#undef FUNC

#define FUNC(expr)                       \
  f64 rs1 = state->fp_regs[inst->rs1].d; \
  f64 rs2 = state->fp_regs[inst->rs2].d; \
  state->gp_regs[inst->rd] = (expr);

static void func_feq_d(state_t *state, inst_t *inst) { FUNC(rs1 == rs2); }

static void func_flt_d(state_t *state, inst_t *inst) { FUNC(rs1 < rs2); }

static void func_fle_d(state_t *state, inst_t *inst) { FUNC(rs1 <= rs2); }

#undef FUNC

static void func_fclass_s(state_t *state, inst_t *inst) {
  state->gp_regs[inst->rd] = f32_classify(state->fp_regs[inst->rs1].f);
}

static void func_fclass_d(state_t *state, inst_t *inst) {
  state->gp_regs[inst->rd] = f64_classify(state->fp_regs[inst->rs1].d);
}

static void func_fcvt_l_s(state_t *state, inst_t *inst) {
  state->gp_regs[inst->rd] = (i64)llrintf(state->fp_regs[inst->rs1].f);
}

static void func_fcvt_lu_s(state_t *state, inst_t *inst) {
  state->gp_regs[inst->rd] = (u64)llrintf(state->fp_regs[inst->rs1].f);
}

static void func_fcvt_l_d(state_t *state, inst_t *inst) {
  state->gp_regs[inst->rd] = (i64)llrint(state->fp_regs[inst->rs1].d);
}

static void func_fcvt_lu_d(state_t *state, inst_t *inst) {
  state->gp_regs[inst->rd] = (u64)llrint(state->fp_regs[inst->rs1].d);
}

static void func_fcvt_s_l(state_t *state, inst_t *inst) {
  state->fp_regs[inst->rd].f = (f32)(i64)state->gp_regs[inst->rs1];
}

static void func_fcvt_s_lu(state_t *state, inst_t *inst) {
  state->fp_regs[inst->rd].f = (f32)(u64)state->gp_regs[inst->rs1];
}

static void func_fcvt_d_l(state_t *state, inst_t *inst) {
  state->fp_regs[inst->rd].d = (f64)(i64)state->gp_regs[inst->rs1];
}

static void func_fcvt_d_lu(state_t *state, inst_t *inst) {
  state->fp_regs[inst->rd].d = (f64)(u64)state->gp_regs[inst->rs1];
}

static void func_fcvt_s_d(state_t *state, inst_t *inst) {
  state->fp_regs[inst->rd].f = (f32)state->fp_regs[inst->rs1].d;
}

static void func_fcvt_d_s(state_t *state, inst_t *inst) {
  state->fp_regs[inst->rd].d = (f64)state->fp_regs[inst->rs1].f;
}

static func_t *funcs[] = {
    func_lb,        func_lh,       func_lw,        func_ld,
    func_lbu,       func_lhu,      func_lwu,       func_fence,
    func_fence_i,   func_addi,     func_slli,      func_slti,
    func_sltiu,     func_xori,     func_srli,      func_srai,
    func_ori,       func_andi,     func_auipc,     func_addiw,
    func_slliw,     func_srliw,    func_sraiw,     func_sb,
    func_sh,        func_sw,       func_sd,        func_add,
    func_sll,       func_slt,      func_sltu,      func_xor,
    func_srl,       func_or,       func_and,       func_mul,
    func_mulh,      func_mulhsu,   func_mulhu,     func_div,
    func_divu,      func_rem,      func_remu,      func_sub,
    func_sra,       func_lui,      func_addw,      func_sllw,
    func_srlw,      func_mulw,     func_divw,      func_divuw,
    func_remw,      func_remuw,    func_subw,      func_sraw,
    func_beq,       func_bne,      func_blt,       func_bge,
    func_bltu,      func_bgeu,     func_jalr,      func_jal,
    func_ecall,     func_csrrw,    func_csrrs,     func_csrrc,
    func_csrrwi,    func_csrrsi,   func_csrrci,    func_flw,
    func_fsw,       func_fmadd_s,  func_fmsub_s,   func_fnmsub_s,
    func_fnmadd_s,  func_fadd_s,   func_fsub_s,    func_fmul_s,
    func_fdiv_s,    func_fsqrt_s,  func_fsgnj_s,   func_fsgnjn_s,
    func_fsgnjx_s,  func_fmin_s,   func_fmax_s,    func_fcvt_w_s,
    func_fcvt_wu_s, func_fmv_x_w,  func_feq_s,     func_flt_s,
    func_fle_s,     func_fclass_s, func_fcvt_s_w,  func_fcvt_s_wu,
    func_fmv_w_x,   func_fcvt_l_s, func_fcvt_lu_s, func_fcvt_s_l,
    func_fcvt_s_lu, func_fld,      func_fsd,       func_fmadd_d,
    func_fmsub_d,   func_fnmsub_d, func_fnmadd_d,  func_fadd_d,
    func_fsub_d,    func_fmul_d,   func_fdiv_d,    func_fsqrt_d,
    func_fsgnj_d,   func_fsgnjn_d, func_fsgnjx_d,  func_fmin_d,
    func_fmax_d,    func_fcvt_s_d, func_fcvt_d_s,  func_feq_d,
    func_flt_d,     func_fle_d,    func_fclass_d,  func_fcvt_w_d,
    func_fcvt_wu_d, func_fcvt_d_w, func_fcvt_d_wu, func_fcvt_l_d,
    func_fcvt_lu_d, func_fmv_x_d,  func_fcvt_d_l,  func_fcvt_d_lu,
    func_fmv_d_x,
};

void exec_block_interp(state_t *state) {
  static inst_t inst = {0};
  while (true) {
    u32 data = *(u32 *)TO_HOST(state->pc);
    inst_decode(&inst, data);

    funcs[inst.type](state, &inst);
    state->gp_regs[zero] = 0;

    if (inst.cont) break;

    state->pc += inst.rvc ? 2 : 4;
  }
}
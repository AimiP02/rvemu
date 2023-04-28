#include "rvemu.h"

#define QUADRANT(data) (((data) >> 0) & 0x3)

/**
 * normal types
 */
#define OPCODE(data) (((data) >> 2) & 0x1f)
#define RD(data) (((data) >> 7) & 0x1f)
#define RS1(data) (((data) >> 15) & 0x1f)
#define RS2(data) (((data) >> 20) & 0x1f)
#define RS3(data) (((data) >> 27) & 0x1f)
#define FUNCT2(data) (((data) >> 25) & 0x3)
#define FUNCT3(data) (((data) >> 12) & 0x7)
#define FUNCT7(data) (((data) >> 25) & 0x7f)
#define IMM116(data) (((data) >> 26) & 0x3f)

static inline inst_t inst_utype_read(u32 data) {
  return (inst_t){
      .imm = (i32)data & 0xfffff000,
      .rd = RD(data),
  };
}

static inline inst_t inst_itype_read(u32 data) {
  return (inst_t){
      .imm = (i32)data >> 20,
      .rs1 = RS1(data),
      .rd = RD(data),
  };
}

static inline inst_t inst_jtype_read(u32 data) {
  u32 imm20 = (data >> 31) & 0x1;
  u32 imm101 = (data >> 21) & 0x3ff;
  u32 imm11 = (data >> 20) & 0x1;
  u32 imm1912 = (data >> 12) & 0xff;

  i32 imm = (imm20 << 20) | (imm1912 << 12) | (imm11 << 11) | (imm101 << 1);
  imm = (imm << 11) >> 11;

  return (inst_t){
      .imm = imm,
      .rd = RD(data),
  };
}

static inline inst_t inst_btype_read(u32 data) {
  u32 imm12 = (data >> 31) & 0x1;
  u32 imm105 = (data >> 25) & 0x3f;
  u32 imm41 = (data >> 8) & 0xf;
  u32 imm11 = (data >> 7) & 0x1;

  i32 imm = (imm12 << 12) | (imm11 << 11) | (imm105 << 5) | (imm41 << 1);
  imm = (imm << 19) >> 19;

  return (inst_t){
      .imm = imm,
      .rs1 = RS1(data),
      .rs2 = RS2(data),
  };
}

static inline inst_t inst_rtype_read(u32 data) {
  return (inst_t){
      .rs1 = RS1(data),
      .rs2 = RS2(data),
      .rd = RD(data),
  };
}

static inline inst_t inst_stype_read(u32 data) {
  u32 imm115 = (data >> 25) & 0x7f;
  u32 imm40 = (data >> 7) & 0x1f;

  i32 imm = (imm115 << 5) | imm40;
  imm = (imm << 20) >> 20;
  return (inst_t){
      .imm = imm,
      .rs1 = RS1(data),
      .rs2 = RS2(data),
  };
}

static inline inst_t inst_csrtype_read(u32 data) {
  return (inst_t){
      .csr = data >> 20,
      .rs1 = RS1(data),
      .rd = RD(data),
  };
}

static inline inst_t inst_fprtype_read(u32 data) {
  return (inst_t){
      .rs1 = RS1(data),
      .rs2 = RS2(data),
      .rs3 = RS3(data),
      .rd = RD(data),
  };
}

/**
 * compressed types
 */
#define COPCODE(data) (((data) >> 13) & 0x7)
#define CFUNCT1(data) (((data) >> 12) & 0x1)
#define CFUNCT2LOW(data) (((data) >> 5) & 0x3)
#define CFUNCT2HIGH(data) (((data) >> 10) & 0x3)
#define RP1(data) (((data) >> 7) & 0x7)
#define RP2(data) (((data) >> 2) & 0x7)
#define RC1(data) (((data) >> 7) & 0x1f)
#define RC2(data) (((data) >> 2) & 0x1f)

static inline inst_t inst_catype_read(u16 data) {
  return (inst_t){
      .rd = RP1(data) + 8,
      .rs2 = RP2(data) + 8,
      .rvc = true,
  };
}

static inline inst_t inst_crtype_read(u16 data) {
  return (inst_t){
      .rs1 = RC1(data),
      .rs2 = RC2(data),
      .rvc = true,
  };
}

static inline inst_t inst_citype_read(u16 data) {
  u32 imm40 = (data >> 2) & 0x1f;
  u32 imm5 = (data >> 12) & 0x1;
  i32 imm = (imm5 << 5) | imm40;
  imm = (imm << 26) >> 26;

  return (inst_t){
      .imm = imm,
      .rd = RC1(data),
      .rvc = true,
  };
}

static inline inst_t inst_citype_read2(u16 data) {
  u32 imm86 = (data >> 2) & 0x7;
  u32 imm43 = (data >> 5) & 0x3;
  u32 imm5 = (data >> 12) & 0x1;

  i32 imm = (imm86 << 6) | (imm43 << 3) | (imm5 << 5);

  return (inst_t){
      .imm = imm,
      .rd = RC1(data),
      .rvc = true,
  };
}

static inline inst_t inst_citype_read3(u16 data) {
  u32 imm5 = (data >> 2) & 0x1;
  u32 imm87 = (data >> 3) & 0x3;
  u32 imm6 = (data >> 5) & 0x1;
  u32 imm4 = (data >> 6) & 0x1;
  u32 imm9 = (data >> 12) & 0x1;

  i32 imm =
      (imm5 << 5) | (imm87 << 7) | (imm6 << 6) | (imm4 << 4) | (imm9 << 9);
  imm = (imm << 22) >> 22;

  return (inst_t){
      .imm = imm,
      .rd = RC1(data),
      .rvc = true,
  };
}

static inline inst_t inst_citype_read4(u16 data) {
  u32 imm5 = (data >> 12) & 0x1;
  u32 imm42 = (data >> 4) & 0x7;
  u32 imm76 = (data >> 2) & 0x3;

  i32 imm = (imm5 << 5) | (imm42 << 2) | (imm76 << 6);

  return (inst_t){
      .imm = imm,
      .rd = RC1(data),
      .rvc = true,
  };
}

static inline inst_t inst_citype_read5(u16 data) {
  u32 imm1612 = (data >> 2) & 0x1f;
  u32 imm17 = (data >> 12) & 0x1;

  i32 imm = (imm1612 << 12) | (imm17 << 17);
  imm = (imm << 14) >> 14;
  return (inst_t){
      .imm = imm,
      .rd = RC1(data),
      .rvc = true,
  };
}

static inline inst_t inst_cbtype_read(u16 data) {
  u32 imm5 = (data >> 2) & 0x1;
  u32 imm21 = (data >> 3) & 0x3;
  u32 imm76 = (data >> 5) & 0x3;
  u32 imm43 = (data >> 10) & 0x3;
  u32 imm8 = (data >> 12) & 0x1;

  i32 imm =
      (imm8 << 8) | (imm76 << 6) | (imm5 << 5) | (imm43 << 3) | (imm21 << 1);
  imm = (imm << 23) >> 23;

  return (inst_t){
      .imm = imm,
      .rs1 = RP1(data) + 8,
      .rvc = true,
  };
}

static inline inst_t inst_cbtype_read2(u16 data) {
  u32 imm40 = (data >> 2) & 0x1f;
  u32 imm5 = (data >> 12) & 0x1;
  i32 imm = (imm5 << 5) | imm40;
  imm = (imm << 26) >> 26;

  return (inst_t){
      .imm = imm,
      .rd = RP1(data) + 8,
      .rvc = true,
  };
}

static inline inst_t inst_cstype_read(u16 data) {
  u32 imm76 = (data >> 5) & 0x3;
  u32 imm53 = (data >> 10) & 0x7;

  i32 imm = ((imm76 << 6) | (imm53 << 3));

  return (inst_t){
      .imm = imm,
      .rs1 = RP1(data) + 8,
      .rs2 = RP2(data) + 8,
      .rvc = true,
  };
}

static inline inst_t inst_cstype_read2(u16 data) {
  u32 imm6 = (data >> 5) & 0x1;
  u32 imm2 = (data >> 6) & 0x1;
  u32 imm53 = (data >> 10) & 0x7;

  i32 imm = ((imm6 << 6) | (imm2 << 2) | (imm53 << 3));

  return (inst_t){
      .imm = imm,
      .rs1 = RP1(data) + 8,
      .rs2 = RP2(data) + 8,
      .rvc = true,
  };
}

static inline inst_t inst_cjtype_read(u16 data) {
  u32 imm5 = (data >> 2) & 0x1;
  u32 imm31 = (data >> 3) & 0x7;
  u32 imm7 = (data >> 6) & 0x1;
  u32 imm6 = (data >> 7) & 0x1;
  u32 imm10 = (data >> 8) & 0x1;
  u32 imm98 = (data >> 9) & 0x3;
  u32 imm4 = (data >> 11) & 0x1;
  u32 imm11 = (data >> 12) & 0x1;

  i32 imm = ((imm5 << 5) | (imm31 << 1) | (imm7 << 7) | (imm6 << 6) |
             (imm10 << 10) | (imm98 << 8) | (imm4 << 4) | (imm11 << 11));
  imm = (imm << 20) >> 20;
  return (inst_t){
      .imm = imm,
      .rvc = true,
  };
}

static inline inst_t inst_cltype_read(u16 data) {
  u32 imm6 = (data >> 5) & 0x1;
  u32 imm2 = (data >> 6) & 0x1;
  u32 imm53 = (data >> 10) & 0x7;

  i32 imm = (imm6 << 6) | (imm2 << 2) | (imm53 << 3);

  return (inst_t){
      .imm = imm,
      .rs1 = RP1(data) + 8,
      .rd = RP2(data) + 8,
      .rvc = true,
  };
}

static inline inst_t inst_cltype_read2(u16 data) {
  u32 imm76 = (data >> 5) & 0x3;
  u32 imm53 = (data >> 10) & 0x7;

  i32 imm = (imm76 << 6) | (imm53 << 3);

  return (inst_t){
      .imm = imm,
      .rs1 = RP1(data) + 8,
      .rd = RP2(data) + 8,
      .rvc = true,
  };
}

static inline inst_t inst_csstype_read(u16 data) {
  u32 imm86 = (data >> 7) & 0x7;
  u32 imm53 = (data >> 10) & 0x7;

  i32 imm = (imm86 << 6) | (imm53 << 3);

  return (inst_t){
      .imm = imm,
      .rs2 = RC2(data),
      .rvc = true,
  };
}

static inline inst_t inst_csstype_read2(u16 data) {
  u32 imm76 = (data >> 7) & 0x3;
  u32 imm52 = (data >> 9) & 0xf;

  i32 imm = (imm76 << 6) | (imm52 << 2);

  return (inst_t){
      .imm = imm,
      .rs2 = RC2(data),
      .rvc = true,
  };
}

static inline inst_t inst_ciwtype_read(u16 data) {
  u32 imm3 = (data >> 5) & 0x1;
  u32 imm2 = (data >> 6) & 0x1;
  u32 imm96 = (data >> 7) & 0xf;
  u32 imm54 = (data >> 11) & 0x3;

  i32 imm = (imm3 << 3) | (imm2 << 2) | (imm96 << 6) | (imm54 << 4);

  return (inst_t){
      .imm = imm,
      .rd = RP2(data) + 8,
      .rvc = true,
  };
}

void inst_decode(inst_t *inst, u32 data) {
  u32 quadrant = QUADRANT(data);
  switch (quadrant) {
    case 0x0: {
      u32 copcode = COPCODE(data);

      switch (copcode) {
        case 0x0: /* C.ADDI4SPN */
          *inst = inst_ciwtype_read(data);
          inst->rs1 = sp;
          inst->type = inst_addi;
          assert(inst->imm != 0);
          return;
        case 0x1: /* C.FLD */
          *inst = inst_cltype_read2(data);
          inst->type = inst_fld;
          return;
        case 0x2: /* C.LW */
          *inst = inst_cltype_read(data);
          inst->type = inst_lw;
          return;
        case 0x3: /* C.LD */
          *inst = inst_cltype_read2(data);
          inst->type = inst_ld;
          return;
        case 0x5: /* C.FSD */
          *inst = inst_cstype_read(data);
          inst->type = inst_fsd;
          return;
        case 0x6: /* C.SW */
          *inst = inst_cstype_read2(data);
          inst->type = inst_sw;
          return;
        case 0x7: /* C.SD */
          *inst = inst_cstype_read(data);
          inst->type = inst_sd;
          return;
        default:
          printf("data: %x\n", data);
          fatal("unimplemented");
      }
    }
      unreachable();
    case 0x1: {
      u32 copcode = COPCODE(data);

      switch (copcode) {
        case 0x0: /* C.ADDI */
          *inst = inst_citype_read(data);
          inst->rs1 = inst->rd;
          inst->type = inst_addi;
          return;
        case 0x1: /* C.ADDIW */
          *inst = inst_citype_read(data);
          assert(inst->rd != 0);
          inst->rs1 = inst->rd;
          inst->type = inst_addiw;
          return;
        case 0x2: /* C.LI */
          *inst = inst_citype_read(data);
          inst->rs1 = zero;
          inst->type = inst_addi;
          return;
        case 0x3: {
          i32 rd = RC1(data);
          if (rd == 2) { /* C.ADDI16SP */
            *inst = inst_citype_read3(data);
            assert(inst->imm != 0);
            inst->rs1 = inst->rd;
            inst->type = inst_addi;
            return;
          } else { /* C.LUI */
            *inst = inst_citype_read5(data);
            assert(inst->imm != 0);
            inst->type = inst_lui;
            return;
          }
        }
          unreachable();
        case 0x4: {
          u32 cfunct2high = CFUNCT2HIGH(data);

          switch (cfunct2high) {
            case 0x0:   /* C.SRLI */
            case 0x1:   /* C.SRAI */
            case 0x2: { /* C.ANDI */
              *inst = inst_cbtype_read2(data);
              inst->rs1 = inst->rd;

              if (cfunct2high == 0x0) {
                inst->type = inst_srli;
              } else if (cfunct2high == 0x1) {
                inst->type = inst_srai;
              } else {
                inst->type = inst_andi;
              }
              return;
            }
              unreachable();
            case 0x3: {
              u32 cfunct1 = CFUNCT1(data);

              switch (cfunct1) {
                case 0x0: {
                  u32 cfunct2low = CFUNCT2LOW(data);

                  *inst = inst_catype_read(data);
                  inst->rs1 = inst->rd;

                  switch (cfunct2low) {
                    case 0x0: /* C.SUB */
                      inst->type = inst_sub;
                      break;
                    case 0x1: /* C.XOR */
                      inst->type = inst_xor;
                      break;
                    case 0x2: /* C.OR */
                      inst->type = inst_or;
                      break;
                    case 0x3: /* C.AND */
                      inst->type = inst_and;
                      break;
                    default:
                      unreachable();
                  }
                  return;
                }
                  unreachable();
                case 0x1: {
                  u32 cfunct2low = CFUNCT2LOW(data);

                  *inst = inst_catype_read(data);
                  inst->rs1 = inst->rd;

                  switch (cfunct2low) {
                    case 0x0: /* C.SUBW */
                      inst->type = inst_subw;
                      break;
                    case 0x1: /* C.ADDW */
                      inst->type = inst_addw;
                      break;
                    default:
                      unreachable();
                  }
                  return;
                }
                  unreachable();
                default:
                  unreachable();
              }
            }
              unreachable();
            default:
              unreachable();
          }
        }
          unreachable();
        case 0x5: /* C.J */
          *inst = inst_cjtype_read(data);
          inst->rd = zero;
          inst->type = inst_jal;
          inst->cont = true;
          return;
        case 0x6: /* C.BEQZ */
        case 0x7: /* C.BNEZ */
          *inst = inst_cbtype_read(data);
          inst->rs2 = zero;
          inst->type = copcode == 0x6 ? inst_beq : inst_bne;
          return;
        default:
          fatal("unrecognized copcode");
      }
    }
      unreachable();
    case 0x2: {
      u32 copcode = COPCODE(data);
      switch (copcode) {
        case 0x0: /* C.SLLI */
          *inst = inst_citype_read(data);
          inst->rs1 = inst->rd;
          inst->type = inst_slli;
          return;
        case 0x1: /* C.FLDSP */
          *inst = inst_citype_read2(data);
          inst->rs1 = sp;
          inst->type = inst_fld;
          return;
        case 0x2: /* C.LWSP */
          *inst = inst_citype_read4(data);
          assert(inst->rd != 0);
          inst->rs1 = sp;
          inst->type = inst_lw;
          return;
        case 0x3: /* C.LDSP */
          *inst = inst_citype_read2(data);
          assert(inst->rd != 0);
          inst->rs1 = sp;
          inst->type = inst_ld;
          return;
        case 0x4: {
          u32 cfunct1 = CFUNCT1(data);

          switch (cfunct1) {
            case 0x0: {
              *inst = inst_crtype_read(data);

              if (inst->rs2 == 0) { /* C.JR */
                assert(inst->rs1 != 0);
                inst->rd = zero;
                inst->type = inst_jalr;
                inst->cont = true;
              } else { /* C.MV */
                inst->rd = inst->rs1;
                inst->rs1 = zero;
                inst->type = inst_add;
              }
              return;
            }
              unreachable();
            case 0x1: {
              *inst = inst_crtype_read(data);
              if (inst->rs1 == 0 && inst->rs2 == 0) { /* C.EBREAK */
                fatal("unimplmented");
              } else if (inst->rs2 == 0) { /* C.JALR */
                inst->rd = ra;
                inst->type = inst_jalr;
                inst->cont = true;
              } else { /* C.ADD */
                inst->rd = inst->rs1;
                inst->type = inst_add;
              }
              return;
            }
              unreachable();
            default:
              unreachable();
          }
        }
          unreachable();
        case 0x5: /* C.FSDSP */
          *inst = inst_csstype_read(data);
          inst->rs1 = sp;
          inst->type = inst_fsd;
          return;
        case 0x6: /* C.SWSP */
          *inst = inst_csstype_read2(data);
          inst->rs1 = sp;
          inst->type = inst_sw;
          return;
        case 0x7: /* C.SDSP */
          *inst = inst_csstype_read(data);
          inst->rs1 = sp;
          inst->type = inst_sd;
          return;
        default:
          fatal("unrecognized copcode");
      }
    }
      unreachable();
    case 0x3: {
      u32 opcode = OPCODE(data);
      switch (opcode) {
        case 0x0: {
          u32 funct3 = FUNCT3(data);

          *inst = inst_itype_read(data);
          switch (funct3) {
            case 0x0: /* LB */
              inst->type = inst_lb;
              return;
            case 0x1: /* LH */
              inst->type = inst_lh;
              return;
            case 0x2: /* LW */
              inst->type = inst_lw;
              return;
            case 0x3: /* LD */
              inst->type = inst_ld;
              return;
            case 0x4: /* LBU */
              inst->type = inst_lbu;
              return;
            case 0x5: /* LHU */
              inst->type = inst_lhu;
              return;
            case 0x6: /* LWU */
              inst->type = inst_lwu;
              return;
            default:
              unreachable();
          }
        }
          unreachable();
        case 0x1: {
          u32 funct3 = FUNCT3(data);

          *inst = inst_itype_read(data);
          switch (funct3) {
            case 0x2: /* FLW */
              inst->type = inst_flw;
              return;
            case 0x3: /* FLD */
              inst->type = inst_fld;
              return;
            default:
              unreachable();
          }
        }
          unreachable();
        case 0x3: {
          u32 funct3 = FUNCT3(data);

          switch (funct3) {
            case 0x0: { /* FENCE */
              inst_t _inst = {0};
              *inst = _inst;
              inst->type = inst_fence;
              return;
            }
            case 0x1: { /* FENCE.I */
              inst_t _inst = {0};
              *inst = _inst;
              inst->type = inst_fence_i;
              return;
            }
            default:
              unreachable();
          }
        }
          unreachable();
        case 0x4: {
          u32 funct3 = FUNCT3(data);

          *inst = inst_itype_read(data);
          switch (funct3) {
            case 0x0: /* ADDI */
              inst->type = inst_addi;
              return;
            case 0x1: {
              u32 imm116 = IMM116(data);
              if (imm116 == 0) { /* SLLI */
                inst->type = inst_slli;
              } else {
                unreachable();
              }
              return;
            }
              unreachable();
            case 0x2: /* SLTI */
              inst->type = inst_slti;
              return;
            case 0x3: /* SLTIU */
              inst->type = inst_sltiu;
              return;
            case 0x4: /* XORI */
              inst->type = inst_xori;
              return;
            case 0x5: {
              u32 imm116 = IMM116(data);

              if (imm116 == 0x0) { /* SRLI */
                inst->type = inst_srli;
              } else if (imm116 == 0x10) { /* SRAI */
                inst->type = inst_srai;
              } else {
                unreachable();
              }
              return;
            }
              unreachable();
            case 0x6: /* ORI */
              inst->type = inst_ori;
              return;
            case 0x7: /* ANDI */
              inst->type = inst_andi;
              return;
            default:
              fatal("unrecognized funct3");
          }
        }
          unreachable();
        case 0x5: /* AUIPC */
          *inst = inst_utype_read(data);
          inst->type = inst_auipc;
          return;
        case 0x6: {
          u32 funct3 = FUNCT3(data);
          u32 funct7 = FUNCT7(data);

          *inst = inst_itype_read(data);

          switch (funct3) {
            case 0x0: /* ADDIW */
              inst->type = inst_addiw;
              return;
            case 0x1: /* SLLIW */
              assert(funct7 == 0);
              inst->type = inst_slliw;
              return;
            case 0x5: {
              switch (funct7) {
                case 0x0: /* SRLIW */
                  inst->type = inst_srliw;
                  return;
                case 0x20: /* SRAIW */
                  inst->type = inst_sraiw;
                  return;
                default:
                  unreachable();
              }
            }
              unreachable();
            default:
              fatal("unimplemented");
          }
        }
          unreachable();
        case 0x8: {
          u32 funct3 = FUNCT3(data);

          *inst = inst_stype_read(data);
          switch (funct3) {
            case 0x0: /* SB */
              inst->type = inst_sb;
              return;
            case 0x1: /* SH */
              inst->type = inst_sh;
              return;
            case 0x2: /* SW */
              inst->type = inst_sw;
              return;
            case 0x3: /* SD */
              inst->type = inst_sd;
              return;
            default:
              unreachable();
          }
        }
          unreachable();
        case 0x9: {
          u32 funct3 = FUNCT3(data);

          *inst = inst_stype_read(data);
          switch (funct3) {
            case 0x2: /* FSW */
              inst->type = inst_fsw;
              return;
            case 0x3: /* FSD */
              inst->type = inst_fsd;
              return;
            default:
              unreachable();
          }
        }
          unreachable();
        case 0xc: {
          *inst = inst_rtype_read(data);

          u32 funct3 = FUNCT3(data);
          u32 funct7 = FUNCT7(data);

          switch (funct7) {
            case 0x0: {
              switch (funct3) {
                case 0x0: /* ADD */
                  inst->type = inst_add;
                  return;
                case 0x1: /* SLL */
                  inst->type = inst_sll;
                  return;
                case 0x2: /* SLT */
                  inst->type = inst_slt;
                  return;
                case 0x3: /* SLTU */
                  inst->type = inst_sltu;
                  return;
                case 0x4: /* XOR */
                  inst->type = inst_xor;
                  return;
                case 0x5: /* SRL */
                  inst->type = inst_srl;
                  return;
                case 0x6: /* OR */
                  inst->type = inst_or;
                  return;
                case 0x7: /* AND */
                  inst->type = inst_and;
                  return;
                default:
                  unreachable();
              }
            }
              unreachable();
            case 0x1: {
              switch (funct3) {
                case 0x0: /* MUL */
                  inst->type = inst_mul;
                  return;
                case 0x1: /* MULH */
                  inst->type = inst_mulh;
                  return;
                case 0x2: /* MULHSU */
                  inst->type = inst_mulhsu;
                  return;
                case 0x3: /* MULHU */
                  inst->type = inst_mulhu;
                  return;
                case 0x4: /* DIV */
                  inst->type = inst_div;
                  return;
                case 0x5: /* DIVU */
                  inst->type = inst_divu;
                  return;
                case 0x6: /* REM */
                  inst->type = inst_rem;
                  return;
                case 0x7: /* REMU */
                  inst->type = inst_remu;
                  return;
                default:
                  unreachable();
              }
            }
              unreachable();
            case 0x20: {
              switch (funct3) {
                case 0x0: /* SUB */
                  inst->type = inst_sub;
                  return;
                case 0x5: /* SRA */
                  inst->type = inst_sra;
                  return;
                default:
                  unreachable();
              }
            }
              unreachable();
            default:
              unreachable();
          }
        }
          unreachable();
        case 0xd: /* LUI */
          *inst = inst_utype_read(data);
          inst->type = inst_lui;
          return;
        case 0xe: {
          *inst = inst_rtype_read(data);

          u32 funct3 = FUNCT3(data);
          u32 funct7 = FUNCT7(data);

          switch (funct7) {
            case 0x0: {
              switch (funct3) {
                case 0x0: /* ADDW */
                  inst->type = inst_addw;
                  return;
                case 0x1: /* SLLW */
                  inst->type = inst_sllw;
                  return;
                case 0x5: /* SRLW */
                  inst->type = inst_srlw;
                  return;
                default:
                  unreachable();
              }
            }
              unreachable();
            case 0x1: {
              switch (funct3) {
                case 0x0: /* MULW */
                  inst->type = inst_mulw;
                  return;
                case 0x4: /* DIVW */
                  inst->type = inst_divw;
                  return;
                case 0x5: /* DIVUW */
                  inst->type = inst_divuw;
                  return;
                case 0x6: /* REMW */
                  inst->type = inst_remw;
                  return;
                case 0x7: /* REMUW */
                  inst->type = inst_remuw;
                  return;
                default:
                  unreachable();
              }
            }
              unreachable();
            case 0x20: {
              switch (funct3) {
                case 0x0: /* SUBW */
                  inst->type = inst_subw;
                  return;
                case 0x5: /* SRAW */
                  inst->type = inst_sraw;
                  return;
                default:
                  unreachable();
              }
            }
              unreachable();
            default:
              unreachable();
          }
        }
          unreachable();
        case 0x10: {
          u32 funct2 = FUNCT2(data);

          *inst = inst_fprtype_read(data);
          switch (funct2) {
            case 0x0: /* FMADD.S */
              inst->type = inst_fmadd_s;
              return;
            case 0x1: /* FMADD.D */
              inst->type = inst_fmadd_d;
              return;
            default:
              unreachable();
          }
        }
          unreachable();
        case 0x11: {
          u32 funct2 = FUNCT2(data);

          *inst = inst_fprtype_read(data);
          switch (funct2) {
            case 0x0: /* FMSUB.S */
              inst->type = inst_fmsub_s;
              return;
            case 0x1: /* FMSUB.D */
              inst->type = inst_fmsub_d;
              return;
            default:
              unreachable();
          }
        }
          unreachable();
        case 0x12: {
          u32 funct2 = FUNCT2(data);

          *inst = inst_fprtype_read(data);
          switch (funct2) {
            case 0x0: /* FNMSUB.S */
              inst->type = inst_fnmsub_s;
              return;
            case 0x1: /* FNMSUB.D */
              inst->type = inst_fnmsub_d;
              return;
            default:
              unreachable();
          }
        }
          unreachable();
        case 0x13: {
          u32 funct2 = FUNCT2(data);

          *inst = inst_fprtype_read(data);
          switch (funct2) {
            case 0x0: /* FNMADD.S */
              inst->type = inst_fnmadd_s;
              return;
            case 0x1: /* FNMADD.D */
              inst->type = inst_fnmadd_d;
              return;
            default:
              unreachable();
          }
        }
          unreachable();
        case 0x14: {
          u32 funct7 = FUNCT7(data);

          *inst = inst_rtype_read(data);
          switch (funct7) {
            case 0x0: /* FADD.S */
              inst->type = inst_fadd_s;
              return;
            case 0x1: /* FADD.D */
              inst->type = inst_fadd_d;
              return;
            case 0x4: /* FSUB.S */
              inst->type = inst_fsub_s;
              return;
            case 0x5: /* FSUB.D */
              inst->type = inst_fsub_d;
              return;
            case 0x8: /* FMUL.S */
              inst->type = inst_fmul_s;
              return;
            case 0x9: /* FMUL.D */
              inst->type = inst_fmul_d;
              return;
            case 0xc: /* FDIV.S */
              inst->type = inst_fdiv_s;
              return;
            case 0xd: /* FDIV.D */
              inst->type = inst_fdiv_d;
              return;
            case 0x10: {
              u32 funct3 = FUNCT3(data);

              switch (funct3) {
                case 0x0: /* FSGNJ.S */
                  inst->type = inst_fsgnj_s;
                  return;
                case 0x1: /* FSGNJN.S */
                  inst->type = inst_fsgnjn_s;
                  return;
                case 0x2: /* FSGNJX.S */
                  inst->type = inst_fsgnjx_s;
                  return;
                default:
                  unreachable();
              }
            }
              unreachable();
            case 0x11: {
              u32 funct3 = FUNCT3(data);

              switch (funct3) {
                case 0x0: /* FSGNJ.D */
                  inst->type = inst_fsgnj_d;
                  return;
                case 0x1: /* FSGNJN.D */
                  inst->type = inst_fsgnjn_d;
                  return;
                case 0x2: /* FSGNJX.D */
                  inst->type = inst_fsgnjx_d;
                  return;
                default:
                  unreachable();
              }
            }
              unreachable();
            case 0x14: {
              u32 funct3 = FUNCT3(data);

              switch (funct3) {
                case 0x0: /* FMIN.S */
                  inst->type = inst_fmin_s;
                  return;
                case 0x1: /* FMAX.S */
                  inst->type = inst_fmax_s;
                  return;
                default:
                  unreachable();
              }
            }
              unreachable();
            case 0x15: {
              u32 funct3 = FUNCT3(data);

              switch (funct3) {
                case 0x0: /* FMIN.D */
                  inst->type = inst_fmin_d;
                  return;
                case 0x1: /* FMAX.D */
                  inst->type = inst_fmax_d;
                  return;
                default:
                  unreachable();
              }
            }
              unreachable();
            case 0x20: /* FCVT.S.D */
              assert(RS2(data) == 1);
              inst->type = inst_fcvt_s_d;
              return;
            case 0x21: /* FCVT.D.S */
              assert(RS2(data) == 0);
              inst->type = inst_fcvt_d_s;
              return;
            case 0x2c: /* FSQRT.S */
              assert(inst->rs2 == 0);
              inst->type = inst_fsqrt_s;
              return;
            case 0x2d: /* FSQRT.D */
              assert(inst->rs2 == 0);
              inst->type = inst_fsqrt_d;
              return;
            case 0x50: {
              u32 funct3 = FUNCT3(data);

              switch (funct3) {
                case 0x0: /* FLE.S */
                  inst->type = inst_fle_s;
                  return;
                case 0x1: /* FLT.S */
                  inst->type = inst_flt_s;
                  return;
                case 0x2: /* FEQ.S */
                  inst->type = inst_feq_s;
                  return;
                default:
                  unreachable();
              }
            }
              unreachable();
            case 0x51: {
              u32 funct3 = FUNCT3(data);

              switch (funct3) {
                case 0x0: /* FLE.D */
                  inst->type = inst_fle_d;
                  return;
                case 0x1: /* FLT.D */
                  inst->type = inst_flt_d;
                  return;
                case 0x2: /* FEQ.D */
                  inst->type = inst_feq_d;
                  return;
                default:
                  unreachable();
              }
            }
              unreachable();
            case 0x60: {
              u32 rs2 = RS2(data);

              switch (rs2) {
                case 0x0: /* FCVT.W.S */
                  inst->type = inst_fcvt_w_s;
                  return;
                case 0x1: /* FCVT.WU.S */
                  inst->type = inst_fcvt_wu_s;
                  return;
                case 0x2: /* FCVT.L.S */
                  inst->type = inst_fcvt_l_s;
                  return;
                case 0x3: /* FCVT.LU.S */
                  inst->type = inst_fcvt_lu_s;
                  return;
                default:
                  unreachable();
              }
            }
              unreachable();
            case 0x61: {
              u32 rs2 = RS2(data);

              switch (rs2) {
                case 0x0: /* FCVT.W.D */
                  inst->type = inst_fcvt_w_d;
                  return;
                case 0x1: /* FCVT.WU.D */
                  inst->type = inst_fcvt_wu_d;
                  return;
                case 0x2: /* FCVT.L.D */
                  inst->type = inst_fcvt_l_d;
                  return;
                case 0x3: /* FCVT.LU.D */
                  inst->type = inst_fcvt_lu_d;
                  return;
                default:
                  unreachable();
              }
            }
              unreachable();
            case 0x68: {
              u32 rs2 = RS2(data);

              switch (rs2) {
                case 0x0: /* FCVT.S.W */
                  inst->type = inst_fcvt_s_w;
                  return;
                case 0x1: /* FCVT.S.WU */
                  inst->type = inst_fcvt_s_wu;
                  return;
                case 0x2: /* FCVT.S.L */
                  inst->type = inst_fcvt_s_l;
                  return;
                case 0x3: /* FCVT.S.LU */
                  inst->type = inst_fcvt_s_lu;
                  return;
                default:
                  unreachable();
              }
            }
              unreachable();
            case 0x69: {
              u32 rs2 = RS2(data);

              switch (rs2) {
                case 0x0: /* FCVT.D.W */
                  inst->type = inst_fcvt_d_w;
                  return;
                case 0x1: /* FCVT.D.WU */
                  inst->type = inst_fcvt_d_wu;
                  return;
                case 0x2: /* FCVT.D.L */
                  inst->type = inst_fcvt_d_l;
                  return;
                case 0x3: /* FCVT.D.LU */
                  inst->type = inst_fcvt_d_lu;
                  return;
                default:
                  unreachable();
              }
            }
              unreachable();
            case 0x70: {
              assert(RS2(data) == 0);
              u32 funct3 = FUNCT3(data);

              switch (funct3) {
                case 0x0: /* FMV.X.W */
                  inst->type = inst_fmv_x_w;
                  return;
                case 0x1: /* FCLASS.S */
                  inst->type = inst_fclass_s;
                  return;
                default:
                  unreachable();
              }
            }
              unreachable();
            case 0x71: {
              assert(RS2(data) == 0);
              u32 funct3 = FUNCT3(data);

              switch (funct3) {
                case 0x0: /* FMV.X.D */
                  inst->type = inst_fmv_x_d;
                  return;
                case 0x1: /* FCLASS.D */
                  inst->type = inst_fclass_d;
                  return;
                default:
                  unreachable();
              }
            }
              unreachable();
            case 0x78: /* FMV_W_X */
              assert(RS2(data) == 0 && FUNCT3(data) == 0);
              inst->type = inst_fmv_w_x;
              return;
            case 0x79: /* FMV_D_X */
              assert(RS2(data) == 0 && FUNCT3(data) == 0);
              inst->type = inst_fmv_d_x;
              return;
            default:
              unreachable();
          }
        }
          unreachable();
        case 0x18: {
          *inst = inst_btype_read(data);

          u32 funct3 = FUNCT3(data);
          switch (funct3) {
            case 0x0: /* BEQ */
              inst->type = inst_beq;
              return;
            case 0x1: /* BNE */
              inst->type = inst_bne;
              return;
            case 0x4: /* BLT */
              inst->type = inst_blt;
              return;
            case 0x5: /* BGE */
              inst->type = inst_bge;
              return;
            case 0x6: /* BLTU */
              inst->type = inst_bltu;
              return;
            case 0x7: /* BGEU */
              inst->type = inst_bgeu;
              return;
            default:
              unreachable();
          }
        }
          unreachable();
        case 0x19: /* JALR */
          *inst = inst_itype_read(data);
          inst->type = inst_jalr;
          inst->cont = true;
          return;
        case 0x1b: /* JAL */
          *inst = inst_jtype_read(data);
          inst->type = inst_jal;
          inst->cont = true;
          return;
        case 0x1c: {
          if (data == 0x73) { /* ECALL */
            inst->type = inst_ecall;
            inst->cont = true;
            return;
          }

          u32 funct3 = FUNCT3(data);
          *inst = inst_csrtype_read(data);
          switch (funct3) {
            case 0x1: /* CSRRW */
              inst->type = inst_csrrw;
              return;
            case 0x2: /* CSRRS */
              inst->type = inst_csrrs;
              return;
            case 0x3: /* CSRRC */
              inst->type = inst_csrrc;
              return;
            case 0x5: /* CSRRWI */
              inst->type = inst_csrrwi;
              return;
            case 0x6: /* CSRRSI */
              inst->type = inst_csrrsi;
              return;
            case 0x7: /* CSRRCI */
              inst->type = inst_csrrci;
              return;
            default:
              unreachable();
          }
        }
          unreachable();
        default:
          unreachable();
      }
    }
      unreachable();
    default:
      unreachable();
  }
}
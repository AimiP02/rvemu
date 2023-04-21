#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#include "elfdef.h"
#include "types.h"

/*
    some utils
*/
#define todo(msg) \
  (fprintf(stderr, "warning: %s:%d [TODO] %s\n", __FILE__, __LINE__, msg))
#define fatalf(fmt, ...)                                                       \
  (fprintf(stderr, "fatal: %s:%d " fmt "\n", __FILE__, __LINE__, __VA_ARGS__), \
   exit(1))
#define fatal(msg) fatalf("%s", msg)
#define unreachable() (fatal("unreachable"), __builtin_unreachable())

#define ROUNDDOWN(x, k) ((x) & -(k))
#define ROUNDUP(x, k)   (((x) + (k)-1) & -(k))
#define MIN(x, y)       ((y) > (x) ? (x) : (y))
#define MAX(x, y)       ((y) < (x) ? (x) : (y))

#define ARRAY_SIZE(x)   (sizeof(x)/sizeof((x)[0]))

#define GUEST_MEMORY_OFFSET 0x088800000000ULL

#define TO_HOST(addr)  (addr + GUEST_MEMORY_OFFSET)
#define TO_GUEST(addr) (addr - GUEST_MEMORY_OFFSET)

/*
    State
*/
enum exit_reason_t {
  none,
  direct_branch,
  indirect_branch,
  ecall,
};

typedef struct {
  enum exit_reason_t exit_reason;
  u64 gp_regs[32];
  u64 pc;
} state_t;

/*
    Instruction
*/

enum inst_type_t {
  inst_addi,
  num_insts,
};

typedef struct {
  i8 rd;
  i8 rs1;
  i8 rs2;
  i32 imm;
  enum inst_type_t type;
  bool rvc;
  bool cont;
} inst_t;

void inst_decode(inst_t *inst, u32 data);
void exec_block_interp(state_t *state);

/*
    MMU
*/
typedef struct {
  u64 entry;
  u64 host_alloc;
  u64 alloc;
  u64 base;
} mmu_t;

void mmu_load_elf(mmu_t *mmu, int fd);

/*
    Machine
*/
typedef struct {
  state_t state;
  mmu_t mmu;
} machine_t;

void machine_load_program(machine_t *m, char *prog);
enum exit_reason_t machine_step(machine_t *m);
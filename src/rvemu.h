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

/*
    State
*/
typedef struct {
  u64 gp_regs[32];
  u64 pc;
} state_t;

/*
    MMU
*/
typedef struct {
  u64 entry;
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
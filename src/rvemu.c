#include "rvemu.h"

#include <assert.h>

int main(int argc, char *argv[]) {
  assert(argc > 1);

  machine_t machine;
  machine_load_program(&machine, argv[1]);

  printf("host alloc: 0x%llx\n", TO_HOST(machine.mmu.entry));
  printf("machine address: 0x%lx\n", (u64)&machine);

  return 0;
}
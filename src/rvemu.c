#include "rvemu.h"

#include <assert.h>

int main(int argc, char *argv[]) {
  assert(argc == 2);

  machine_t machine;
  machine_load_program(&machine, argv[1]);

  printf("host alloc: 0x%llx\n", TO_HOST(machine.mmu.entry));
  printf("machine address: 0x%lx\n", (u64)&machine);

  while (true) {
    enum exit_reason_t reason = machine_step(&machine);
    if (reason == ecall) {
      printf("exit reason: %d\n", reason);
      break;
    }
  }

  return 0;
}
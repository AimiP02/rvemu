#include "rvemu.h"
#include "types.h"

void mmu_load_elf(mmu_t *mmu, int fd) {
  u8 buf[sizeof(elf64_ehdr_t)];
  FILE *file = fdopen(fd, "rb");
  if (fread(buf, 1, sizeof(elf64_ehdr_t), file) != sizeof(elf64_ehdr_t)) {
    fatal("File too small to be an ELF file");
  }

  elf64_ehdr_t *ehdr = (elf64_ehdr_t *)buf;

  if (*(u32 *)ehdr != *(u32 *)ELFMAG) {
    fatal("Not an ELF file");
  }

  if (ehdr->e_ident[EI_CLASS] != ELFCLASS64 || ehdr->e_machine != EM_RISCV) {
    fatal("Not a 64-bit ELF file");
  }

  mmu->entry = (u64)ehdr->e_entry;
}
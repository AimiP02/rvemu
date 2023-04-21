#include <sys/mman.h>

#include "rvemu.h"
#include "types.h"

static int flag_to_mmap_prot(u32 flag) {
  int prot = 0;
  if (flag & PF_R) {
    prot |= PROT_READ;
  }
  if (flag & PF_W) {
    prot |= PROT_WRITE;
  }
  if (flag & PF_X) {
    prot |= PROT_EXEC;
  }
  return prot;
}

static void load_phdr(elf64_phdr_t *phdr, elf64_ehdr_t *ehdr, i64 i,
                      FILE *file) {
  if (fseek(file, ehdr->e_phoff + i * ehdr->e_phentsize, SEEK_SET) != 0) {
    fatal("Failed to seek to program header");
  }

  if (fread((void *)phdr, 1, sizeof(elf64_phdr_t), file) !=
      sizeof(elf64_phdr_t)) {
    fatal("Failed to read program header, file too small");
  }
}

static void mmu_load_segment(mmu_t *mmu, elf64_phdr_t *phdr, int fd) {
  // load guest program into host program memory, no page manager yet;
  int page_size = getpagesize();
  u64 offset = phdr->p_offset;
  u64 vaddr = TO_HOST(phdr->p_vaddr);
  u64 aligned_addr = ROUNDDOWN(vaddr, page_size);
  u64 file_size = phdr->p_filesz + (vaddr - aligned_addr);
  u64 mem_size = phdr->p_memsz + (vaddr - aligned_addr);
  int prot = flag_to_mmap_prot(phdr->p_flags);

  // printf(
  //     "offset: 0x%lx, vaddr: 0x%lx, aligned_addr: 0x%lx, file_size: 0x%lx,"
  //     " mem_size: 0x%lx, prot: %d\n",
  //     offset, vaddr, aligned_addr, file_size, mem_size, prot);

  u64 addr =
      (u64)mmap((void *)aligned_addr, file_size, prot, MAP_PRIVATE | MAP_FIXED,
                fd, ROUNDDOWN(offset, page_size));

  assert(addr == aligned_addr);

  u64 remaining_bss =
      ROUNDUP(mem_size, page_size) - ROUNDUP(file_size, page_size);

  if (remaining_bss > 0) {
    // map .bss memory
    addr = (u64)mmap((void *)(aligned_addr + ROUNDUP(file_size, page_size)),
                     remaining_bss, prot,
                     MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS, -1, 0);
    assert(addr == aligned_addr + ROUNDUP(file_size, page_size));
  }

  mmu->host_alloc =
      MAX(mmu->host_alloc, (aligned_addr + ROUNDUP(mem_size, page_size)));
  mmu->base = mmu->alloc = TO_GUEST(mmu->host_alloc);
}

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

  elf64_phdr_t phdr;
  for (i64 i = 0; i < ehdr->e_phnum; i++) {
    load_phdr(&phdr, ehdr, i, file);

    if (phdr.p_type == PT_LOAD) {
      mmu_load_segment(mmu, &phdr, fd);
    }
  }
}
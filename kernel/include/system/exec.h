#ifndef H_EXEC
#define H_EXEC

#include <lib/barelib.h>

#define EI_NIDENT 16
#define ELFCLASS64 2
#define ELFDATA2LSB 1
#define ELF_CURRENT 1
#define EM_RISCV 243
#define PT_LOAD 1
#define USER_REGION_SIZE 0x400000UL

typedef struct {
    byte e_ident[EI_NIDENT];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} elf64_ehdr;

_Static_assert(sizeof(elf64_ehdr) == 64, "elf64_ehdr must be 64 bytes");

typedef struct {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
} elf64_phdr;

_Static_assert(sizeof(elf64_phdr) == 56, "elf64_phdr must be 56 bytes");

int32_t exec_elf(const char*);

#endif
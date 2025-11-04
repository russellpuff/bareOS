#ifndef H_EXEC
#define H_EXEC

#include <lib/barelib.h>

/*
 * The following constants describe the subset of the ELF64 specification that
 * the kernel currently understands.  They are intentionally small – we only
 * load 64-bit little-endian RISC-V executables that follow the base ABI.
 */

#define EI_NIDENT 16
#define ELFCLASS64 2
#define ELFDATA2LSB 1
#define ELF_CURRENT 1
#define EM_RISCV 243
#define PT_LOAD 1
#define USER_REGION_SIZE 0x400000UL

 /*
  * ELF file layout overview
  * ------------------------
  * An ELF binary begins with a fixed-size header (`elf64_ehdr`).  The first
  * sixteen bytes – stored in `e_ident` – encode several identification values
  * such as the ELF magic number, whether the file is 32/64 bit, and the data
  * endianness.  The remaining fields provide offsets that let us locate the
  * rest of the file:
  *
  *  - `e_entry` is the virtual address where execution begins once the file
  *    is loaded.
  *  - `e_phoff` points to the program header table, an array of `elf64_phdr`
  *    structures that describe segments to map into memory.
  *  - `e_shoff` points to the section header table.  The current loader does
  *    not use these entries, but the offsets still exist to maintain the
  *    standard ELF layout.
  *  - The various `*_size` and `*_num` fields describe how large the header
  *    tables are so the loader can iterate over them safely.
  */

typedef struct {
    byte e_ident[EI_NIDENT];    /* Identification bytes, including ELF magic           */
    uint16_t e_type;            /* Object file type (executable, relocatable, etc.)    */
    uint16_t e_machine;         /* Instruction set architecture of the binary          */
    uint32_t e_version;         /* ELF version for the rest of the header              */
    uint64_t e_entry;           /* First instruction address once the image is loaded  */
    uint64_t e_phoff;           /* File offset to the program header table             */
    uint64_t e_shoff;           /* File offset to the section header table             */
    uint32_t e_flags;           /* Target-specific flags (unused currently)            */
    uint16_t e_ehsize;          /* Total size in bytes of this ELF header              */
    uint16_t e_phentsize;       /* Size in bytes of each program header entry          */
    uint16_t e_phnum;           /* Number of entries in the program header table       */
    uint16_t e_shentsize;       /* Size in bytes of each section header entry          */
    uint16_t e_shnum;           /* Number of entries in the section header table       */
    uint16_t e_shstrndx;        /* Index of section-name string table                  */
} elf64_ehdr;

_Static_assert(sizeof(elf64_ehdr) == 64, "elf64_ehdr must be 64 bytes");

/*
 * Each program header (a `PT_LOAD` entry) tells the loader how to map a file
 * segment into virtual memory.  The loader will allocate pages covering the
 * interval [`p_vaddr`, `p_vaddr + p_memsz`) and copy `p_filesz` bytes starting
 * from `p_offset` within the ELF file.  Remaining bytes up to `p_memsz` are
 * zero-initialised to support BSS segments.  The alignment field allows the
 * kernel to verify that segments respect architectural alignment requirements.
 */

typedef struct {
    uint32_t p_type;    /* Segment type (PT_LOAD entries describe loadable code/data) */
    uint32_t p_flags;   /* Access permissions (read/write/execute) for the segment    */
    uint64_t p_offset;  /* File offset where this segment's bytes begin               */
    uint64_t p_vaddr;   /* Virtual address to map the first byte of the segment       */
    uint64_t p_paddr;   /* Physical address hint (unused)                             */
    uint64_t p_filesz;  /* Number of initialised bytes stored in the file             */
    uint64_t p_memsz;   /* Total size of the segment once mapped (includes BSS)       */
    uint64_t p_align;   /* Required alignment for the mapping                         */
} elf64_phdr;

_Static_assert(sizeof(elf64_phdr) == 56, "elf64_phdr must be 56 bytes");

int32_t exec(const char*);

#endif
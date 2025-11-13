#ifndef H_EXEC
#define H_EXEC

#include <barelib.h>


/* The following constants describe the subset of the ELF64 specification that *
 * the kernel currently understands.  They are intentionally small – we only   *
 * load 64-bit little-endian RISC-V executables that follow the hyperspecific  *
 * requirements arbitrarily chosen by the project head.                        */

#define EI_NIDENT 16  /* Size of the ident[] array in the ELF header */
#define ELFCLASS64 2  /* ELF class = 64-bit objects                  */
#define ELFDATAENC 1  /* Data encoding = little-endian               */
#define ELF_CURRENT 1 /* ELF header version, always 1 apparently     */
#define EM_RISCV 243  /* Machine type = RISC-V                       */
#define PT_LOAD 1     /* Program header type for loadable segments   */
#define USER_REGION_SIZE 0x400000UL


 /* An ELF binary begins with a fixed-size header (`elf_hdr`).  The first		 *
  * sixteen bytes – stored in `ident` – encode several identification values	 *
  * such as the ELF magic number, whether the file is 32/64 bit, and the data	 *
  * endianness.  The remaining fields provide offsets that let us locate the	 *
  * rest of the file:															 *
  *																				 *
  *  - `entry_point` is the virtual address where execution begins once the file *
  *    is loaded.																 *
  *  - `pht_offset` points to the program header table, an array of `pht_entry`	 *
  *    structures that describe segments to map into memory.					 *
  *  - `sht_offset` points to the section header table.  The current loader does *
  *    not use these entries, but the offsets still exist to maintain the		 *
  *    standard ELF layout.														 *
  *  - The various `*_size` and `*_num` fields describe how large the header	 *
  *    tables are so the loader can iterate over them safely.                    */
	  

typedef struct {
	uint8_t ident[EI_NIDENT]; /* Identification bytes, including ELF magic           */
	uint16_t type;            /* Object file type (executable, relocatable, etc.)    */
	uint16_t machine;         /* Instruction set architecture of the binary          */
	uint32_t version;         /* ELF version for the rest of the header              */
	uint64_t entry_point;     /* First instruction address once the image is loaded  */
	uint64_t pht_offset;      /* File offset to the program header table             */
	uint64_t sht_offset;      /* File offset to the section header table             */
	uint32_t flags;           /* Target-specific flags (unused currently)            */
	uint16_t header_sz;       /* Total size in bytes of this ELF header              */
	uint16_t pht_entrysz;     /* Size in bytes of each program header entry          */
	uint16_t pht_count;       /* Number of entries in the program header table       */
	uint16_t sht_entrysz;     /* Size in bytes of each section header entry          */
	uint16_t sht_count;       /* Number of entries in the section header table       */
	uint16_t snst_idx;        /* Index of section-name string table                  */
} elf_hdr;

_Static_assert(sizeof(elf_hdr) == 64, "elf_hdr must be 64 bytes");


/* Each program header (a `PT_LOAD` entry) tells the loader how to map a file	  *
 * segment into virtual memory.  The loader will allocate pages covering the	  *
 * interval [`virt_addr`, `virt_addr + mem_sz`) and copy `file_sz` bytes starting *
 * from `offset` within the ELF file.  Remaining bytes up to `mem_sz` are         *
 * zero-initialised to support BSS segments.  The alignment field allows the	  *
 * kernel to verify that segments respect architectural alignment requirements.   */

typedef struct {
	uint32_t type;      /* Segment type (PT_LOAD entries describe loadable code/data) */
	uint32_t flags;     /* Access permissions (read/write/execute) for the segment    */
	uint64_t offset;    /* File offset where this segment's bytes begin               */
	uint64_t virt_addr; /* Virtual address to map the first byte of the segment       */
	uint64_t phys_addr; /* Physical address hint (unused)                             */
	uint64_t file_sz;   /* Number of initialised bytes stored in the file             */
	uint64_t mem_sz;    /* Total size of the segment once mapped (includes BSS)       */
	uint64_t align;     /* Required alignment for the mapping                         */
} pht_entry;

_Static_assert(sizeof(pht_entry) == 56, "pht_entry must be 56 bytes");

int32_t exec(const char*, const char*);

#endif

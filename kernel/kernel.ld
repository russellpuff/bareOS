OUTPUT_ARCH(riscv)
ENTRY(_start)

/*
 * This file tells the compiler what address in memory to place each function
 * and global variable.  It also allows us to save certain memory addresses
 * into variables for use in C code (see _mmap_text_start)
 */


/*
  MEMORY MAP Globals
  
  text_start  - Label at the address of the first instruction in the text segment
  text_end    - Label at the address directly after text segment

  data_start  - Label at the address of the first global variable in the data segment
  data_end    - Label at the address directly after the data segment

  bss_start   - Label at the address of the first uninitialized global variable in the bss segment
  bss_end     - Label at the address directly after the bss segment

  mem_start   - Label at the address of the first address of the global heap/stack segment
  mem_end     - Label at the address directly after the heap/stack segment (This differs from text/data/bss_end)

  _kmap_write_start - Addresses below this point should be READ ONLY
  _kmap_global_ptr  - Address to initialize the RISC-V 'gp' register
*/

MEMORY {
	_sysmem(RX!I) : ORIGIN =0x80000000, LENGTH =200K
	_writable(WA) : ORIGIN =0x80000000+200K-4, LENGTH =127M
}

SECTIONS {
	.text : {
	    PROVIDE(text_start = ABSOLUTE(.));
	    *(.text.entry)
	    *(.text .text.*)
	    . = ALIGN(4);
	    PROVIDE(text_end = ABSOLUTE(.));
	} > _sysmem

	.rodata BLOCK(0x1000) : {
	    PROVIDE(data_start = ABSOLUTE(.));
	    *(.srodata .srodata.*)
	    *(.rodata .rodata.*)
	} > _sysmem

	.data : {
	    PROVIDE(_kmap_write_start = ABSOLUTE(.));
	    *(.sdata .sdata.*)
	    *(.data .data.*)
	    PROVIDE(data_end = ABSOLUTE(.));
	} > _writable

	.bss : {
        PROVIDE(bss_start = ABSOLUTE(.));
        *(.sbss .sbss.*)
        *(.bss .bss.*)
        PROVIDE(bss_end = ABSOLUTE(.));
    } > _writable

	. = ALIGN(0x1000); /* Page align after bss */

	PROVIDE(ramdisk_start = ABSOLUTE(.));
	. += 0x200000; /* Reserve boot ramdisk region. Remove this when ramdisk becomes physical disk. */

	PROVIDE(krsvd = ABSOLUTE(.));
    . += 0x4000; /* Add scratch space for starting kernel stack and reserved utility region. */

	PROVIDE(_kmap_global_ptr = data_start + 0x0800);
    PROVIDE(mem_start = ABSOLUTE(.)); /* mem_start == kernel stack pointer starting location, also start of free memory */
    PROVIDE(mem_end = ORIGIN(_writable) + (LENGTH(_writable)) - 1);
}

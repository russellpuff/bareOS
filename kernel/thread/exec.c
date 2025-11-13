#include <system/exec.h>
#include <fs/fs.h>
#include <lib/bareio.h>
#include <mm/malloc.h>
#include <mm/vm.h>
#include <system/thread.h>
#include <system/semaphore.h>
#include <util/string.h>

/* Checks the ELF header for values that match our expectations. 
   A mixture of mandatory checks for any ELF and our oc donut steel
   ELF specifications (whatever is a constant in exec.h.)           */
static bool is_supported_elf(const elf_hdr* hdr, uint32_t file_size) {
	if (hdr == NULL || file_size < sizeof(elf_hdr)) return false;
	if (hdr->ident[0] != 0x7f || hdr->ident[1] != 'E' || hdr->ident[2] != 'L' || hdr->ident[3] != 'F') return false;
	if (hdr->ident[4] != ELFCLASS64 || hdr->ident[5] != ELFDATAENC) return false;
	if (hdr->ident[6] != ELF_CURRENT) return false;
	if (hdr->machine != EM_RISCV) return false;
	if (hdr->version != ELF_CURRENT) return false;
	if (hdr->pht_count == 0) return false;
	if (hdr->pht_entrysz != sizeof(pht_entry)) return false;
	if (hdr->header_sz != sizeof(elf_hdr)) return false;
	if (hdr->entry_point >= USER_REGION_SIZE) return false;
	if (hdr->pht_offset > file_size) return false;

	uint64_t table_bytes = (uint64_t)hdr->pht_count * hdr->pht_entrysz;
	if (hdr->pht_offset + table_bytes > file_size) return false;
	
	return true;
}

/* Validation for each section, makes sure the segment is both *
 * real and true, and also doesn't map past the fixed VA range *
 * we set for processes. In the future, we won't have to worry *
 * since the page allocator will give more pages.              */
static bool validate_segment(const pht_entry* ph, uint32_t file_size) {
	if (ph->type != PT_LOAD) return true;
	if (ph->file_sz > ph->mem_sz) return false;
	if (ph->offset > file_size) return false;
	if (ph->file_sz > (uint64_t)file_size - ph->offset) return false;
	if (ph->virt_addr >= USER_REGION_SIZE) return false;
	if (ph->mem_sz > USER_REGION_SIZE) return false;
	if (ph->virt_addr > USER_REGION_SIZE - ph->mem_sz) return false;
	return true;
}

/* If we abort partway through, wipe the partially allocated thread table entry */
static void cleanup_failed_thread(uint32_t tid) {
	free_process_pages(tid);
	thread_table[tid].root_ppn = NULL;
	thread_table[tid].state = TH_FREE;
	thread_table[tid].sem = create_sem(0);
	thread_table[tid].tf = NULL;
	thread_table[tid].ctx = NULL;
	thread_table[tid].stackptr = NULL;
}

/* Function copies args from arg_line to the user process stack so 	     *
 * argc and argv become available to main(). This follows a specific	 *
 * convention based on real standards, except we're not using envp		 *
 * or any of the other optional standards seen in specific architecture. *
 *																		 *
 * It's very simple, left->right == lower addresses -> higher addresses	 *
 * [&argv[0]][&argv[1]][...][NULL][padding][argv[0]][argv[1]][...]		 *
 *																		 *
 * The padding is necessary so the sp stays 16 byte aligned per spec,	 *
 * and a null character separates the argv array from its values. 		 *
 *																		 *
 * On full systems, envp and others are typically in between these two.	 *
 * Also, argc is placed at the lowest address, normally user ELF files	 *
 * need to read the argc and get the start of the argv array by looking	 *
 * just above the initial stack pointer (placed below all this) but we	 *
 * don't need to do that because the trapframe structure allows 		 *
 * manipulating the initial state of the program when main is called.    */
static void process_args(const char* program_name, const char* arg_line, int32_t* c, uint64_t* v, uint64_t root_ppn) {
	const char* args = arg_line != NULL ? arg_line : "";
	uint64_t nlen = strlen(program_name);
	uint64_t alen = strlen(args);
	uint64_t vlen;
	int32_t argc = 0;
	char* argv = NULL;
	bool allocated = false;

	if (alen != 0) {
		/* Get space for the full buffer */
		vlen = nlen + alen + 2;
		argv = malloc(vlen);
		allocated = true;
		memcpy(argv, program_name, nlen + 1);
		memcpy(argv + nlen + 1, args, alen + 1);

		/* Go through and replace spaces with null terms  *
		 * to turn into a sequence of strings, then count *
		 * null terms to gather argc.					  *
		 * Note: if there is unwanted whitespace, this	  *
		 * will count them as 0 length argv values.		  *
		 * Not this function's job to check that.         */
		for (uint64_t i = 0; i < vlen; ++i) {
			if (argv[i] == ' ') argv[i] = '\0';
			if (argv[i] == '\0') ++argc;
		}
	}
	else {
		/* No arg_line: only program_name, one argument */
		vlen = nlen + 1;
		argv = (char*)program_name;
		argc = 1;
		}

	/* Compute virtual addresses of the strings (argv**) for storing in the array (argv*) */
	uint64_t pointer_count = (uint64_t)argc + 1; /* argv[argc] must be NULL */
	uint64_t arrsize = sizeof(uint64_t) * pointer_count;
	uint64_t* argptrs = malloc(arrsize);
	uint64_t end_va = USER_REGION_SIZE - 1; /* End of the VA range these strings will appear in */

	/* Padding required so sp is 16 byte aligned */
	uint64_t base = arrsize + vlen;
	uint64_t pad = (16 - (base % 16)) % 16;
	uint64_t total = base + pad;
	uint64_t start_va = end_va - total + 1;
	uint64_t strings_base_va = start_va + arrsize + pad;

	char* final = malloc(total);
	memset(final, 0, total);
	memcpy(final + arrsize + pad, argv, vlen);

	/* Walk through and calculate the VA values of the argv array */
	char* walker = argv;
	for (int32_t i = 0; i < argc; ++i) {
		argptrs[i] = strings_base_va + (uint64_t)(walker - argv);
		walker += strlen(walker) + 1;
	}
	argptrs[argc] = 0;
	memcpy(final, argptrs, arrsize);

	copy_to_user(root_ppn, start_va, final, total);

	if (allocated) free(argv);
	free(argptrs);
	free(final);

	*c = argc;
	*v = start_va;
}

/* Validates and attempts to execute an externally-compiled  *
 * ELF (Executable and Linkable Format) file. For now, must	 *
 * be compiled from the user/ folder using the provided		 *
 * build scripts or this will either reject it or start with *
 * undefined behavior. 										 *
 * For now, only executes programs that end with ".elf" and	 *
 * are in the /bin directory                                 */
int32_t exec(const char* program_name, const char* arg_line) {
	if (program_name == NULL) return -2;

	/* Try to read an elf with the same name as 'program_name' */
	const char suffix[] = ".elf";
	uint32_t base_len = strlen(program_name);
	char filename[FILENAME_LEN];
	memcpy(filename, program_name, base_len); /* Get full filename */
	memcpy(filename + base_len, suffix, 5); /* Add suffix */

	dirent_t bin;
	if (!dir_child_exists(boot_fsd->super.root_dirent, "bin", &bin)) {
		kprintf("%s: /bin directory is missing\n", program_name);
		return -1;
	}

	FILE f;
	f.fd = (FD)-1;
	if (open(filename, &f, bin) != 0) return -2;

	if (f.inode.size == 0) {
		close(&f);
		kprintf("%s: file is empty\n", program_name);
		return -1;
	}

	/* Copy entire elf into memory. Should be okay since they're all small.
	   In the future we'll read it in chunks.                               */
	byte* elf = malloc(f.inode.size);
	if (elf == NULL) {
		close(&f);
		kprintf("%s: insufficient memory\n", program_name);
		return -1;
	}

	int32_t bytes_read = read(&f, elf, f.inode.size);
	close(&f);
	if (bytes_read < 0 || (uint32_t)bytes_read != f.inode.size) {
		free(elf);
		kprintf("%s: failed to read image\n", program_name);
		return -1;
	}

	/* Uses structs that mirror the expected layout to validate the header */
	const elf_hdr* hdr = (const elf_hdr*)elf;
	if (!is_supported_elf(hdr, f.inode.size)) {
		free(elf);
		kprintf("%s: invalid ELF header\n", program_name);
		return -1;
	}

	/* Program header table can be treated like an array of pht_entry objects */
	const pht_entry* ph_table = (const pht_entry*)(elf + hdr->pht_offset);
	uint16_t ph_count = hdr->pht_count;

	for (uint16_t i = 0; i < ph_count; ++i) {
		if (ph_table[i].type != PT_LOAD) continue;
		if (!validate_segment(&ph_table[i], f.inode.size)) {
			free(elf);
			kprintf("%s: invalid program segment\n", program_name);
			return -1;
		}
	}

	/* create_thread can conveniently load from the entry point once in virtual space */
	int32_t tid = create_thread((void*)hdr->entry_point, MODE_U);
	if (tid < 0) {
		free(elf);
		kprintf("%s: unable to create process\n", program_name);
		return -1;
	}

	/* Once we have the root_ppn of the thread, we can start writing data to the pages */
	thread_t* thread = &thread_table[tid];
	if (zero_user(thread->root_ppn, 0, USER_REGION_SIZE) < 0) {
		cleanup_failed_thread(tid);
		free(elf);
		kprintf("%s: failed to prepare address space\n", program_name);
		return -1;
	}

	/* For each pht entry, we'll work through and copy its data given the offsets provided */
	for (uint16_t i = 0; i < ph_count; ++i) {
		const pht_entry* ph = &ph_table[i];
		if (ph->type != PT_LOAD) continue; /* We're only doing PT_LOAD segments, there's others but they're unsupported */

		if (ph->file_sz > 0) { 
			/* Thankfully the pht entry does all the math for us, we just need to know where to write it */
			if (copy_to_user(thread->root_ppn, ph->virt_addr, elf + ph->offset, ph->file_sz) < 0) {
				cleanup_failed_thread(tid);
				free(elf);
				kprintf("%s: failed to load segment\n", program_name);
				return -1;
			}
		}

		/* Some pht entries need extra space beyond what we write, so we zero them 
		   generally to be used as the .bss for this segment, per requirements     */
		if (ph->mem_sz > ph->file_sz) {
			uint64_t zero_len = ph->mem_sz - ph->file_sz;
			if (zero_user(thread->root_ppn, ph->virt_addr + ph->file_sz, zero_len) < 0) {
				cleanup_failed_thread(tid);
				free(elf);
				kprintf("%s: failed to zero segment\n", program_name);
				return -1;
			}
		}
	}

	process_args(program_name, arg_line, (int32_t*)&thread->tf->a0, &thread->tf->a1, thread->root_ppn);
	thread->tf->a0 = (uint32_t)thread->tf->a0; /* Ensure argc is zero-extended after writing via 32-bit pointer */
	thread->tf->sp = thread->tf->a1; /* Set sp to &argv[0] so instructions go below it */

	free(elf);
	return tid;
}

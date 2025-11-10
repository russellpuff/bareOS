#include <system/exec.h>
#include <fs/fs.h>
#include <lib/bareio.h>
#include <mm/malloc.h>
#include <mm/vm.h>
#include <system/thread.h>
#include <system/semaphore.h>
#include <util/string.h>

static bool is_supported_elf(const elf64_ehdr* hdr, uint32_t file_size) {
	if (hdr == NULL || file_size < sizeof(elf64_ehdr)) return false;
	if (hdr->e_ident[0] != 0x7f || hdr->e_ident[1] != 'E' || hdr->e_ident[2] != 'L' || hdr->e_ident[3] != 'F') return false;
	if (hdr->e_ident[4] != ELFCLASS64 || hdr->e_ident[5] != ELFDATA2LSB) return false;
	if (hdr->e_ident[6] != ELF_CURRENT) return false;
	if (hdr->e_machine != EM_RISCV) return false;
	if (hdr->e_version != ELF_CURRENT) return false;
	if (hdr->e_phnum == 0) return false;
	if (hdr->e_phentsize != sizeof(elf64_phdr)) return false;
	if (hdr->e_ehsize != sizeof(elf64_ehdr)) return false;
	if (hdr->e_entry >= USER_REGION_SIZE) return false;
	if (hdr->e_phoff > file_size) return false;

	uint64_t table_bytes = (uint64_t)hdr->e_phnum * hdr->e_phentsize;
	if (hdr->e_phoff + table_bytes > file_size) return false;
	
	return true;
}

static bool validate_segment(const elf64_phdr* ph, uint32_t file_size) {
	if (ph->p_type != PT_LOAD) return true;
	if (ph->p_filesz > ph->p_memsz) return false;
	if (ph->p_offset > file_size) return false;
	if (ph->p_filesz > (uint64_t)file_size - ph->p_offset) return false;
	if (ph->p_vaddr >= USER_REGION_SIZE) return false;
	if (ph->p_memsz > USER_REGION_SIZE) return false;
	if (ph->p_vaddr > USER_REGION_SIZE - ph->p_memsz) return false;
	return true;
}

static void cleanup_failed_thread(uint32_t tid) {
	free_process_pages(tid);
	thread_table[tid].root_ppn = NULL;
	thread_table[tid].state = TH_FREE;
	thread_table[tid].sem = create_sem(0);
	thread_table[tid].argptr = NULL;
	thread_table[tid].tf = NULL;
	thread_table[tid].ctx = NULL;
	thread_table[tid].stackptr = NULL;
}

int32_t exec(const char* program_name) {
	if (program_name == NULL) return -2;

	const char suffix[] = ".elf";
	uint32_t base_len = strlen(program_name);
	char filename[FILENAME_LEN];
	memcpy(filename, program_name, base_len); /* Get full filename */
	memcpy(filename + base_len, suffix, 5); /* Add suffix */

	dirent_t bin;
	if (!dir_child_exists(boot_fsd->super.root_dirent, "bin", &bin)) {
		kprintf("%s: /bin directory is missing\n");
		return -1;
	}

	FILE f;
	f.fd = (FD)-1;
	open(filename, &f, bin);
	if (f.fd == (FD)-1) {
		return -2;
	}

	if (f.inode.size == 0) {
		close(&f);
		kprintf("%s: file is empty\n", program_name);
		return -1;
	}

	byte* buffer = malloc(f.inode.size);
	if (buffer == NULL) {
		close(&f);
		kprintf("%s: insufficient memory\n", program_name);
		return -1;
	}

	int32_t bytes_read = read(&f, buffer, f.inode.size);
	close(&f);
	if (bytes_read < 0 || (uint32_t)bytes_read != f.inode.size) {
		free(buffer);
		kprintf("%s: failed to read image\n", program_name);
		return -1;
	}

	const elf64_ehdr* hdr = (const elf64_ehdr*)buffer;
	if (!is_supported_elf(hdr, f.inode.size)) {
		free(buffer);
		kprintf("%s: invalid ELF header\n", program_name);
		return -1;
	}

	const elf64_phdr* ph_table = (const elf64_phdr*)(buffer + hdr->e_phoff);
	uint16_t ph_count = hdr->e_phnum;

	for (uint16_t i = 0; i < ph_count; ++i) {
		if (ph_table[i].p_type != PT_LOAD) continue;
		if (!validate_segment(&ph_table[i], f.inode.size)) {
			free(buffer);
			kprintf("%s: invalid program segment\n", program_name);
			return -1;
		}
	}

	int32_t tid = create_thread((void*)hdr->e_entry, "", 0, MODE_U);
	if (tid < 0) {
		free(buffer);
		kprintf("%s: unable to create process\n", program_name);
		return -1;
	}

	thread_t* thread = &thread_table[tid];
	if (zero_user(thread->root_ppn, 0, USER_REGION_SIZE) < 0) {
		cleanup_failed_thread(tid);
		free(buffer);
		kprintf("%s: failed to prepare address space\n", program_name);
		return -1;
	}

	for (uint16_t i = 0; i < ph_count; ++i) {
		const elf64_phdr* ph = &ph_table[i];
		if (ph->p_type != PT_LOAD) continue;

		if (ph->p_filesz > 0) {
			if (copy_to_user(thread->root_ppn, ph->p_vaddr, buffer + ph->p_offset, ph->p_filesz) < 0) {
				cleanup_failed_thread(tid);
				free(buffer);
				kprintf("%s: failed to load segment\n", program_name);
				return -1;
			}
		}

		if (ph->p_memsz > ph->p_filesz) {
			uint64_t zero_len = ph->p_memsz - ph->p_filesz;
			if (zero_user(thread->root_ppn, ph->p_vaddr + ph->p_filesz, zero_len) < 0) {
				cleanup_failed_thread(tid);
				free(buffer);
				kprintf("%s: failed to zero segment\n", program_name);
				return -1;
			}
		}
	}

	free(buffer);
	return tid;
}

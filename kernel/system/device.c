#include <fs/fs.h>
#include <mm/kmalloc.h>
#include <lib/bareio.h>
#include <system/thread.h>
#include <device/rtc.h>
#include <dev/ecall.h>
#include <dev/time.h>
#include <util/string.h>

thread_t* proc;

//
// Open handlers
//

/* Called by: 
		fopen()
		fcreate()
		mkdir()
		getdir()
*/
static uint32_t disk_dev_open(byte* options) {
	disk_dev_opts* opts = (disk_dev_opts*)options;
	switch (opts->mode) {
		case FILE_OPEN:
			return (uint32_t)open((const char*)opts->buff_in, opts->file, proc->cwd);
		case FILE_CREATE:
			return (uint32_t)create((const char*)opts->buff_in, proc->cwd);
		case DIR_CREATE:
			return (uint32_t)mk_dir((const char*)opts->buff_in, proc->cwd, (dirent_t*)opts->buff_out);
		case DIR_OPEN: /* Poorly named alias for fetching a directory and possibly switching cwd to it */
			directory_t* target = (directory_t*)opts->buff_out;
			bool chdir = *(bool*)opts->buff_in;
			const char* path = (const char*)opts->buff_in + sizeof(bool);
			uint8_t status = resolve_dir(path, proc->cwd, &target->dir);
			if (status != 0) return status;
			if (target->dir.type != EN_DIR) return 3;
			char dirname[FILENAME_LEN];
			status = path_to_name(path, dirname);
			if (status == 5) {
				uint32_t len = strlen(target->dir.name);
				memcpy(dirname, target->dir.name, len + 1);
			}
			else if (status == 3 || status == 4) {
				target->dir = status == 3 ? boot_fsd->super.root_dirent : thread_table[current_thread].cwd;
				const char* ref = status == 3 ? boot_fsd->super.root_dirent.name : thread_table[current_thread].cwd.name;
				uint32_t len = strlen(ref);
				memcpy(dirname, ref, len + 1);
			}
			if ((status == 1 || status == 2) && strcmp(dirname, target->dir.name)) {
				dirent_t candidate;
				if (!dir_child_exists(target->dir, dirname, &candidate)) return 2; /* Target missing */
				if (candidate.type != EN_DIR) return 3; /* Target is not a directory */
				target->dir = candidate;
			}
			if (status == 0) return 1;

			/* Update process cwd when the resolved directory differs or when we need to seed the initial path */
			bool dir_changed = proc->cwd.inode != target->dir.inode;
			if ((dir_changed || *target->path == '\0') && chdir) {
				char* pos = dirent_path_expand(target->dir, target->path);
				uint32_t l = strlen(pos) + 1;
				memcpy(target->path, pos, l);
				proc->cwd = target->dir;
			}
			return 0;
		default: break;
	}
	return (uint32_t)-1;
}

static uint32_t handle_ecall_open(uint32_t device, byte* options) {
	switch (device) {
		case UART_DEV_NUM: return (uint32_t)-1;
		case DISK_DEV_NUM: return disk_dev_open(options);
		default: break;
	}
	return 0;
}

//
// Close handlers
//

/* Called by:
		fclose()
*/
static uint32_t disk_dev_close(byte* options) {
	disk_dev_opts* opts = (disk_dev_opts*)options;
	return close(opts->file);
	return (uint32_t)-1;
}

static uint32_t handle_ecall_close(uint32_t device, byte* options) {
	switch (device) {
		case UART_DEV_NUM: return (uint32_t)-1;
		case DISK_DEV_NUM: return disk_dev_close(options);
		default: break;
	}
	return 0;
}

//
// Read handlers
//

/* Called by:
		fread()
		readdir()
		rtc_read()
*/
static uint32_t rtc_dev_read(byte* options) {
	rtc_dev_opts* opts = (rtc_dev_opts*)options;
	if (opts->type == GET_SEC) {
		uint64_t* time = (uint64_t*)opts->buffer;
		*time = rtc_read_seconds();
	}
	else { /* GET_TZ */
		tzrule* lt = (tzrule*)opts->buffer;
		*lt = localtime;
	}
	return 0;
}

static uint32_t disk_dev_read(byte* options) {
	disk_dev_opts* opts = (disk_dev_opts*)options;
	switch (opts->mode) {
		case FILE_READ: return read(opts->file, opts->buff_out, opts->length);
		case DIR_READ:
			if (opts->length == 0) return 0;
			dirent_t parent;
			uint8_t status = resolve_dir((const char*)opts->buff_in, proc->cwd, &parent);
			if (status != 0) return status;
			if (parent.type != EN_DIR) return 3;

			char dirname[FILENAME_LEN];
			status = path_to_name((const char*)opts->buff_in, dirname);
			if (status == 0) return 1;
			if (status == 3) parent = boot_fsd->super.root_dirent;
			if (status == 4) parent = thread_table[current_thread].cwd;
			if (status == 5) {
				/* Parent already canonicalized by resolve_dir */
				memcpy(dirname, parent.name, strlen(parent.name) + 1);
			}
			if ((status == 1 || status == 2) && strcmp(dirname, parent.name)) return 2;
			
			dir_iter_t iter;
			dirent_t* children = (dirent_t*)opts->buff_out;
			dir_open(parent.inode, &iter);
			uint32_t count = 0;
			for (; count < opts->length && dir_next(&iter, children) == 1; ++count, ++children);
			return count;
		default: break;
	}
	return (uint32_t)-1;
}

static uint32_t uart_dev_read(byte* options) {
	uart_dev_opts* opts = (uart_dev_opts*)options;
	if (opts->buffer == NULL || opts->length == 0) return 0;
	return get_line((char*)opts->buffer, opts->length);
}

static uint32_t handle_ecall_read(uint32_t device, byte* options) {
	switch (device) {
		case UART_DEV_NUM: return uart_dev_read(options);
		case DISK_DEV_NUM: return disk_dev_read(options);
		case RTC_DEV_NUM: return rtc_dev_read(options);
		default: break;
	}
	return 0;
}

//
// Write handlers
//

/* Called by:
		fwrite()
		fdelete()
		rmdir()
		rtc_chtz()
*/
static uint32_t rtc_dev_write(byte* options) {
	rtc_dev_opts* opts = (rtc_dev_opts*)options;
	uint8_t code = change_localtime((const char*)opts->buffer);
	return code;
}

static uint32_t disk_dev_write(byte* options) {
	disk_dev_opts* opts = (disk_dev_opts*)options;
	switch (opts->mode) {
		case FILE_WRITE: return write(opts->file, opts->buff_in, opts->length);
		case FILE_TRUNCATE:
			if (opts->length == 0) { /* Delete the file */
				return (uint32_t)unlink((const char*)opts->buff_in, proc->cwd);
			}
			// No method exists to properly truncate files, don't call this yet.
			return 0;
		case DIR_TRUNCATE:
			if (opts->length == 0) { /* Delete the directory */
				// dir can only be deleted if empty, no -f exists
				return (uint32_t)rm_dir((const char*)opts->buff_in, proc->cwd);
			}
		default: break;
	}
	return (uint32_t)-1;
}

static uint32_t uart_dev_write(byte* options) {
	uart_dev_opts* opts = (uart_dev_opts*)options;
	byte* buff = kmalloc(opts->length + 1);
	memcpy(buff, opts->buffer, opts->length);
	buff[opts->length] = '\0';
	kprintf((char*)buff);
	free(buff);
	return 0;
}

static uint32_t handle_ecall_write(uint32_t device, byte* options) {
	switch (device) {
		case UART_DEV_NUM: return uart_dev_write(options);
		case DISK_DEV_NUM: return disk_dev_write(options);
		case RTC_DEV_NUM: return rtc_dev_write(options);
		default: break;
	}
	return (uint32_t)-1;
}

//
// Entry point
//

uint32_t handle_device_ecall(ecall_number id, uint32_t device, byte* options) {
	proc = &thread_table[current_thread];
	switch (id) {
		case ECALL_OPEN: return handle_ecall_open(device, options);
		case ECALL_CLOSE: return handle_ecall_close(device, options);
		case ECALL_READ: return handle_ecall_read(device, options);
		case ECALL_WRITE: return handle_ecall_write(device, options);
		default: break;
	}
	return (uint32_t)-1;
}

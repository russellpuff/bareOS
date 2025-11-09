#include <lib/barelib.h>
#include <lib/bareio.h>
#include <lib/string.h>
#include <system/thread.h>
#include <system/exec.h>
#include <system/syscall.h>
#include <system/queue.h>
#include <lib/ecall.h>
#include <system/panic.h>
#include <mm/vm.h>
#include <mm/malloc.h>
#include <fs/fs.h>

#define SYSCON_ADDR 0x100000
#define SYSCON_SHUTDOWN 0x5555
#define SYSCON_REBOOT 0x7777
volatile uint64_t signum;
static inline void reserved(void*) { return; }

void (*syscall_table[])(void*) = {
  resched,
  reserved
};

void handle_syscall(uint64_t* frame) {
	(void)frame;
	int32_t table_size = sizeof(syscall_table) / sizeof(uint32_t(*)(void));
	if (signum < table_size)
		syscall_table[signum](&handle_syscall);
}

static uint8_t handle_ecall_spawn(char* name, char* arg) {
	(void)arg; /* unused for now */
	uint8_t ret = 0;
	int32_t tid = exec(name);
	if (tid >= 0) {
		resume_thread(tid);
		ret = join_thread(tid);
	}
	else if (tid == -2) { kprintf("%s: command not found\n", name); }
	else ret = 1;
	return ret;
}

void signal_syscon(uint16_t signal) {
	const char* what = signal == SYSCON_SHUTDOWN ? "shut down" : "reboot";
	krprintf("The system will %s now.\n", what);
	*(uint16_t*)PA_TO_KVA(SYSCON_ADDR) = signal;
	while (1);
}

static void handle_ecall_pwoff(void) {
	signal_syscon(SYSCON_SHUTDOWN);
}

static void handle_ecall_rboot(void) {
	signal_syscon(SYSCON_REBOOT);
}

void handle_ecall(uint64_t* frame_data, uint64_t call_id) {
	trapframe* tf = (trapframe*)frame_data;
	if (tf == NULL)
		return;
	tf->sepc += 4;
	uint32_t result = 0;
	/* TODO: function lookup table */
	switch ((ecall_number)call_id) {
		case ECALL_GDEV: break;
		case ECALL_PWOFF: handle_ecall_pwoff(); break;
		case ECALL_RBOOT: handle_ecall_rboot(); break;
		case ECALL_OPEN:
		case ECALL_CLOSE:
		case ECALL_READ:
		case ECALL_WRITE:
			result = handle_device_ecall((ecall_number)call_id, (uint32_t)tf->a0, (byte*)tf->a1);
			break;
		case ECALL_SPAWN: result = handle_ecall_spawn((char*)tf->a0, (char*)tf->a1); break;
		case ECALL_EXIT: /* Currently assumes a supervisor process didn't call this. They have their own exit strategy. */
			if (thread_table[current_thread].mode == MODE_U) user_thread_exit(tf);
			break;
	}

	tf->a0 = result;
}

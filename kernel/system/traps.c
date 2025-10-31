#include <lib/barelib.h>
#include <lib/bareio.h>
#include <system/thread.h>
#include <system/exec.h>
#include <system/interrupts.h>
#include <system/syscall.h>
#include <system/queue.h>
#include <lib/ecall.h>
#include <system/panic.h>
#include <device/timer.h>
#include <mm/vm.h>
#include <mm/malloc.h>

volatile uint64_t signum;

/*
 *  This file contains code for handling exceptions generated
 *  by the hardware   (see '__traps' in bootstrap.s)
 *
 *  We can add a new syscall by writing a function and adding
 *  it to the 'syscall_table' below.
 */

static inline void reserved(void) { return; }

void (*syscall_table[]) (void) = {
  resched,
  reserved
};

void handle_syscall(uint64_t* frame) {
    (void)frame;
    int32_t table_size = sizeof(syscall_table) / sizeof(uint32_t (*)(void));
    if (signum < table_size)
        syscall_table[signum]();
}

void s_handle_exception(uint64_t* frame) {
   uint64_t cause, tval, epc;
   asm volatile("csrr %0, scause" : "=r"(cause));
   asm volatile("csrr %0, stval"  : "=r"(tval));
   asm volatile("csrr %0, sepc"   : "=r"(epc));

    if ((cause & (1ULL << 63)) == 0) { /* Synchronous exception */
        uint64_t code = cause & 0xfffULL; /* Get exception code */
        /* Placeholder for handling basic page faults without a crash */
        /* 12 = instruction page fault */
        /* 13 = load page fault        */
        /* 15 = store page fault */
        if (code == 12 || code == 13 || code == 15) {
            /* No handler. Just kill the thread. */
            krprintf("Thread %u faulted at %x on code %u\n", current_thread, (uint32_t)tval, code);
            if (ready_list.qnext == &ready_list) {
                panic("Couldn't resched after a fault. There's no other available threads to switch to.\n");
            }
            kill_thread(current_thread);
            signum = RESCHED;
            handle_syscall(frame);
            return;
        }
    }

    kprintf("Unhandled exception: scause=%x stval=%x, sepc=%x\n", cause, tval, epc);
    while (1);
}

void m_handle_exception(void) {
    /* Read the registers in gdb */
    // uint64_t cause, tval, epc;
    // asm volatile("csrr %0, mcause" : "=r"(cause));
    // asm volatile("csrr %0, mtval"  : "=r"(tval));
    // asm volatile("csrr %0, mepc"   : "=r"(epc));

    while (1);
}

//
// ecall handlers
//
static uint32_t handle_ecall_read(uint32_t device, char* target, char* buffer, uint32_t length) {
    (void)device;
    (void)target;
    if (buffer == NULL || length == 0) return 0;
    return get_line(buffer, length);
}

static void handle_ecall_write(uint32_t device, char* target, char* buffer, uint32_t length) {
    (void)device;
    (void)target;
    /* TEMP - Only user-available device is the uart, organizing devices into a table is not supported yet. Keep disk ops kernel-only. */
    /* device == uart */
    /* target == NULL (for uart) */
    /* write: buffer = what to write */
    kuprintf(buffer, length);
}

static byte handle_ecall_spawn(char* name, char* arg) {
    (void)arg; /* unused for now */
    byte ret = 0;
    int32_t tid = exec_elf(name);
    if (tid >= 0) {
        resume_thread(tid);
        ret = join_thread(tid);
    }
    else if (tid == -2) { kprintf("%s: command not found\n", name); }
    else ret = 1;
    return ret;
}

#define SYSCON_ADDR 0x100000
#define SYSCON_SHUTDOWN 0x5555
#define SYSCON_REBOOT 0x7777

void signal_syscon(uint16_t signal) {
    const char* what = signal == SYSCON_SHUTDOWN ? "shut down" : "reboot";
    kprintf("The system will %s now.\n", what);
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
    case ECALL_OPEN: break;
    case ECALL_CLOSE: break;
    case ECALL_READ: result = handle_ecall_read((uint32_t)tf->a0, (char*)tf->a1, (char*)tf->a2, (uint32_t)tf->a3); break;
    case ECALL_WRITE: handle_ecall_write((uint32_t)tf->a0, (char*)tf->a1, (char*)tf->a2, (uint32_t)tf->a3); break;
    case ECALL_SPAWN: result = handle_ecall_spawn((char*)tf->a0, (char*)tf->a1); break;
    case ECALL_EXIT: /* Currently assumes a supervisor process didn't call this. They have their own exit strategy. */
        if (thread_table[current_thread].mode == MODE_U) user_thread_exit(tf);
        break;
    }

    tf->a0 = result;
}
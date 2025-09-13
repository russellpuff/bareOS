#include <lib/bareio.h>
#include <app/shell.h>

#define SYSCON_ADDR 0x100000
#define SYSCON_SHUTDOWN 0x5555
#define SYSCON_REBOOT 0x7777

/* 'builtin_shutdown' and 'builtin_reboot' both write a magic number   * 
 * to QEMU's syscon linked memory address which prompts an emulator    *
 * shutdown or reboot respectively. They currently execute immediately *
 * and kill the whole system.                                          */

void signal_syscon(uint16_t signal) {
    const char* what = signal == SYSCON_SHUTDOWN ? "shut down" : "reboot";
    kprintf("The system will %s now.\n", what);
    *(uint16_t*)SYSCON_ADDR = signal;
    while (1);
}

byte builtin_shutdown(char* arg) {
    signal_syscon(SYSCON_SHUTDOWN);
    return 0; /* Unreachable */
}

byte builtin_reboot(char* arg) {
    signal_syscon(SYSCON_REBOOT);
    return 0; /* Unreachable */
}
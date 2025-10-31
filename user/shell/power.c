#include <lib/io.h>
#include <lib/ecall.h>
#include "shell.h"

/* 'builtin_shutdown' and 'builtin_reboot' both ecall to write a magic     *
 * number to QEMU's syscon linked memory address which prompts an emulator *
 * shutdown or reboot respectively. They currently execute immediately     *
 * and kill the whole system without warning processes.                    */

byte builtin_shutdown(char* arg) {
    ecall_pwoff();
    return 0; /* Unreachable */
}

/* This function doesn't work right now and is not registered as a shell command. */
byte builtin_reboot(char* arg) {
    ecall_rboot();
    return 0; /* Unreachable */
}
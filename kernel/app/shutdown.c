#include <bareio.h>
#include <barelib.h>
#include <shell.h>


/* 'builtin_shutdown' prints a shutdown message and then loops forever. *
 * Since we can't signal QEMU to exit directly, we instead let scons    *
 * wait until it sees this message, and does the signaling itself.      */
byte builtin_shutdown(char* arg) {
    // TODO: broadcast_signal(SIGKILL);
    kprintf("The system will shut down now.\n");
    while(1);
    return 0; /* Unreachable */
}
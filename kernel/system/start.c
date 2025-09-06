#include <barelib.h>
#include <shell.h>
#include <thread.h>

void initialize(void);
void display_kernel_info(void);

/*
 *  This file contains the C code entry point executed by the kernel.
 *  It is called by the bootstrap sequence once the hardware is configured.
 *      (see bootstrap.s)
 */

static void sys_idle() { for(;;) { } }

static void root_thread(void) {
    uint32 idleTID = create_thread(&sys_idle, '\0', 1);
    resume_thread(idleTID);
    uint32 sh = create_thread(&shell, '\0', 1);
    resume_thread(sh);
    join_thread(sh);
    for(;;) { }
}

/*
 *  First c function called
 *  Used to initialize devices before starting steady state behavior
 */
void supervisor_start(void) {
  initialize();
  display_kernel_info();
  root_thread();
}


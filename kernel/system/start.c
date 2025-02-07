#include <barelib.h>
#include <shell.h>

void initialize(void);
void display_kernel_info(void);

/*
 *  This file contains the C code entry point executed by the kernel.
 *  It is called by the bootstrap sequence once the hardware is configured.
 *      (see bootstrap.s)
 */

static void root_thread(void) {
  shell("what goes here? lol");
  while (1);
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


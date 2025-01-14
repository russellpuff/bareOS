#include <barelib.h>
#include <interrupts.h>

void display_kernel_info(void) {
  /* Pre-boot student code goes here */
  /* ------------------------------- */

  

  /* ------------------------------- */
}


/*
 *  This function calls any initialization functions to set up
 *  devices and other systems.
 */
void initialize(void) {
  init_uart();
  init_interrupts();
}

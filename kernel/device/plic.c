/*
 *  This file contains functions for initializing and managing the
 *  'Platform-Level Interrupt Controller' or PLIC.
 *  These functions must be called to allow interrupts to be passed
 *  to the interrupt vector (see '__traps' in bootstrap.s).
 */

#include <lib/barelib.h>
#include <system/interrupts.h>
#include <mm/vm.h>

#define TRAP_EXTERNAL_ENABLE 0x200
#define EXTERNAL_PENDING_ADDR (0xc001000 + (MMU_ENABLED ? KVM_BASE : 0)) 
#define EXTERNAL_THRESH_ADDR  (0xc201000 + (MMU_ENABLED ? KVM_BASE : 0)) 

void uart_handler(void);

/*
 *  This function is called during bootstrapping to enable interrupts
 *  from the Platform Local Interrupt Controller [PLIC] (see bootstrap.s)
 */
void init_plic(void) {
  uint32_t* plic_thresh_addr = (uint32_t*)EXTERNAL_THRESH_ADDR;
  *plic_thresh_addr  = 0x0;
  set_interrupt(TRAP_EXTERNAL_ENABLE);
}


/* 
 * This function is automatically triggered whenver an external 
 * event occurs.  (see '__traps' in bootstrap.s)
 */
s_interrupt handle_plic(void) {
  uint32_t* plic_pending_addr = (uint32_t*)EXTERNAL_PENDING_ADDR;
  if (*plic_pending_addr == 0x400)
      uart_handler();
  *plic_pending_addr = 0x0;
}

/*
 *  This file contains functions for initializing and managing the
 *  'Platform-Level Interrupt Controller' or PLIC.
 *  These functions must be called to allow interrupts to be passed
 *  to the interrupt vector (see '__traps' in bootstrap.s).
 */

#include <lib/barelib.h>
#include <system/interrupts.h>
#include <mm/vm.h>

#define PLIC_ENABLE 0x200
#define PLIC_S_THRESH (0x0C201000UL + (MMU_ENABLED ? KVM_BASE : 0))
#define PLIC_S_CLAIM (0x0C201004UL + (MMU_ENABLED ? KVM_BASE : 0))

void uart_handler(void);

/*
 *  This function is called during bootstrapping to enable interrupts
 *  from the Platform Local Interrupt Controller [PLIC] (see bootstrap.s)
 */
void init_plic(void) {
	volatile uint32_t* const thresh = (volatile uint32_t*)PLIC_S_THRESH;
	*thresh = 0;                          // allow all priorities
	set_m_interrupt(PLIC_ENABLE);
}


/* 
 * This function is automatically triggered whenver an external 
 * event occurs.  (see '__traps' in bootstrap.s)
 */
void handle_plic(void) {
	volatile uint32_t* const claim = (volatile uint32_t*)PLIC_S_CLAIM;
	uint32_t id = *claim;                 
	if (id == 10) /* QEMU virt: UART0 is source ID 10 */
		uart_handler();
	*claim = id;
}

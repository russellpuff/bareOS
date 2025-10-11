#ifndef H_INTERRUPT
#define H_INTERRUPT

#include <lib/barelib.h>

void init_interrupts(void);        /*  Set up supervisor mode interrupt vector     */
uint32_t set_m_interrupt(uint32_t);      /*  Turn on an interrupt for a specific signal  */
uint32_t set_s_interrupt(uint32_t);
uint32_t disable_interrupts(void);   /*  Turn off all interrupts                     */
void restore_interrupts(uint32_t);   /*  Return the interrupts to a given state      */
void acknowledge_interrupt();  /*  Reset a triggered interrupt                 */
void uart_wake_tx(void);
extern volatile uint64_t signum;

#endif

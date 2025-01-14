#ifndef H_INTERRUPT
#define H_INTERRUPT

#include <barelib.h>

void init_interrupts(void);        /*  Set up supervisor mode interrupt vector     */
uint32 set_interrupt(uint32);      /*  Turn on an interrupt for a specific signal  */
uint32 disable_interrupts(void);   /*  Turn off all interrupts                     */
void restore_interrupts(uint32);   /*  Return the interrupts to a given state      */
void acknowledge_interrupt(void);  /*  Reset a triggered interrupt                 */

#endif

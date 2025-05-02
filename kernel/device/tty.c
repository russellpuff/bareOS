#include <barelib.h>
#include <semaphore.h>
#include <interrupts.h>
#include <tty.h>

ring_buffer_t tty_in;
ring_buffer_t tty_out;

/*  Initialize the `tty_in_sem` and `tty_out_sem` semaphores  *
 *  for later TTY calls.                                      */
void init_tty(void) {
  
}

/*  Get a character  from the `tty_in`  buffer and remove  *
 *  it from the circular buffer.  If the buffer is empty,  * 
 *  wait on  the semaphore  for data to be  placed in the  *
 *  buffer by the UART.                                    */
char tty_getc(void) {
  
  return 0;
}

/*  Place a character into the `tty_out` buffer and enable  *
 *  uart interrupts.   If the buffer is  full, wait on the  *
 *  semaphore  until notified  that there  space has  been  *
 *  made in the  buffer by the UART. */
void tty_putc(char ch) {

  
}

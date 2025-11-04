#include <lib/barelib.h>
#include <system/semaphore.h>
#include <system/interrupts.h>
#include <device/tty.h>

ring_buffer_t tty_in;
ring_buffer_t tty_out;

/*  Initialize the `tty_in_sem` and `tty_out_sem` semaphores  *
 *  for later TTY calls.                                      */
void init_tty(void) {
	tty_in.head = 0;
	tty_in.count = 0;
	tty_in.sem = create_sem(0);
	tty_out.head = 0;
	tty_out.count = 0;
	tty_out.sem = create_sem(TTY_BUFFLEN);
}

/*  Get a character  from the `tty_in`  buffer and remove  *
 *  it from the circular buffer.  If the buffer is empty,  * 
 *  wait on  the semaphore  for data to be  placed in the  *
 *  buffer by the UART.                                    */
char tty_getc(void) {
	wait_sem(&tty_in.sem);
	char c = tty_in.buffer[tty_in.head];
	tty_in.head  = (tty_in.head + 1) % TTY_BUFFLEN;
	--tty_in.count;
	return c;
}

/* Helper function bc I'm tired of rewriting this so many times. */
byte put_to_tail(char ch) {
    wait_sem(&tty_out.sem);
    byte needs_wake = (tty_out.count == 0);
    uint32_t tail = (tty_out.head + tty_out.count) % TTY_BUFFLEN;
    tty_out.buffer[tail] = ch;
    ++tty_out.count;
    return needs_wake;
}

/*  Place a character into the `tty_out` buffer and enable  *
 *  uart interrupts.   If the buffer is  full, wait on the  *
 *  semaphore  until notified  that there  space has  been  *
 *  made in the  buffer by the UART. */
void tty_putc(char ch) {
    byte needs_wake = 0;
    if (ch == '\n') needs_wake |= put_to_tail('\r');
    needs_wake |= put_to_tail(ch);
    if (needs_wake) uart_wake_tx();
}

/* Enqueue the backspace erase sequence into the tty while only *
 * waking the UART once for the whole sequence. Reducing lag.  */
void tty_bkspc(void) {
    byte needs_wake = 0;
    needs_wake |= put_to_tail('\b');
    needs_wake |= put_to_tail(' ');
    needs_wake |= put_to_tail('\b');
    if (needs_wake) uart_wake_tx();
}
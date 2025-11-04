#ifndef H_TTY
#define H_TTY

#include <system/semaphore.h>

#define TTY_BUFFLEN 512

typedef struct {
  semaphore_t sem;           /* Semaphore used to lock the ring buffer while waiting for data */
  char buffer[TTY_BUFFLEN];  /* Circular buffer for storing characters                        */
  uint32_t head;               /* Index of the first character available in the ring buffer     */
  uint32_t count;              /* Number of characters in the ring buffer                       */
} ring_buffer_t;

extern ring_buffer_t tty_in;   /* Ring Buffer used for characters recieved from the UART device */
extern ring_buffer_t tty_out;  /* Ring Buffer used for characters sent to the UART device */

void init_tty(void);
char tty_getc(void);
void tty_putc(char);
void tty_bkspc(void);

void set_uart_interrupt(byte);

#endif

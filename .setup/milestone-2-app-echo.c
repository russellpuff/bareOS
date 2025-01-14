#include <bareio.h>
#include <barelib.h>


/*
 * 'builtin_echo' reads in a line of text from the UART
 * and prints it.  It will continue to do this until the
 * line read from the UART is empty (indicated with a \n
 * followed immediately by another \n).
 */
byte builtin_echo(char* arg) {
  return 0;
}

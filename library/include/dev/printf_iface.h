#ifndef H_PRINTF_IFACE
#define H_PRINTF_IFACE

#include <barelib.h>

/* This is an interface file for helping abstract kernel/user implementations *
 * of io functins. Meant only to be used in shared library files that print   */

void printf(const char*, ...);
void sprintf(byte*, const char*, ...);

#endif
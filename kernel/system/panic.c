#include <system/panic.h>
#include <mm/vm.h>

#define UART0_CFG_REG 0x10000000
#define UART0_RW_REG   0x0 
#define UART0_STAT_REG 0x5
#define UART_IDLE     0x20

static char raw_putc(char ch) {
	byte* uart = (byte*)PA_TO_KVA(UART0_CFG_REG);
	while ((uart[UART0_STAT_REG] & UART_IDLE) == 0);     /*  Wait for the UART to be idle  */
	return uart[UART0_RW_REG] = (ch & 0xff);             /*  Send character to the UART    */
}

static void puts(const char* str) {
    while (*str != '\0') raw_putc(*str++);
}

/* TODO: use ksprintf to build a proper dump and then send the printed buffer to puts */
void panic(const char* s) {
	puts(s);
	while (1);
}
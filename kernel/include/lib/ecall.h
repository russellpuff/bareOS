#ifndef H_ECALL
#define H_ECALL

#include <lib/barelib.h>

/* TEMPORARY */
#define UART_DEV_NUM 0
#define DISK_DEV_NUM 1

/* Options for io ecall request */
/* Maybe this should be in io.h but it's not included in the kernel... */
typedef struct {
    uint32_t length;
} io_dev_opts;

/* Enum assigns identifying numbers to different ecalls    *
 * This is based on common linux numbering but not exactly */
typedef enum {
    ECALL_GDEV  = 0,   /* Request a list of available devices   */ /* This is a temp solution until we can enumerate devices properly */
    ECALL_PWOFF = 1,   /* Request immediate shutdown            */ /* This is a temp solution until better power options support      */
    ECALL_RBOOT = 2,   /* Request immediate reboot              */ /* This is a temp solution until better power options support      */
    ECALL_OPEN  = 56,  /* Call the open() function of a device  */
    ECALL_CLOSE = 57,  /* Call the close() function of a device */
    ECALL_READ  = 63,  /* Call the read() function of a device  */
    ECALL_WRITE = 64,  /* Call the write() function of a device */
    ECALL_SPAWN = 92,  /* Spawn a child of the current process  */
    ECALL_EXIT  = 93   /* Exit a user process                   */
} ecall_number;

uint64_t ecall_open(uint32_t, byte*);
uint64_t ecall_close(uint32_t, byte*);
uint64_t ecall_read(uint32_t, byte*, byte*);
uint64_t ecall_write(uint32_t, byte*, byte*);
uint64_t ecall_spawn(char*, char*);
void ecall_pwoff(void);
void ecall_rboot(void);

void handle_ecall(uint64_t*, uint64_t);

#endif
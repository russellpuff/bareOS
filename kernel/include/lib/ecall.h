#ifndef H_ECALL
#define H_ECALL

#include <lib/barelib.h>

typedef enum {
    ECALL_OPEN,
    ECALL_CLOSE,
    ECALL_READ,
    ECALL_WRITE,
    ECALL_SPAWN,
    ECALL_EXIT = 93,
} ecall_number;

uint32_t ecall_open(uint32_t, char*);
uint32_t ecall_close(uint32_t, char*);
uint32_t ecall_read(uint32_t, char*, char*, uint32_t);
uint32_t ecall_write(uint32_t, char*, char*, uint32_t);
uint32_t ecall_spawn(byte*, char*);

void handle_ecall(uint64_t*, uint64_t);

#endif
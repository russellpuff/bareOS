#ifndef H_ECALL
#define H_ECALL

#include <lib/barelib.h>

typedef enum {
    S_ECALL_OPEN,
    S_ECALL_CLOSE,
    S_ECALL_READ,
    S_ECALL_WRITE,
} s_ecall_number;

uint32_t s_ecall_open(uint32_t, char*);
uint32_t s_ecall_close(uint32_t, char*);
uint32_t s_ecall_read(uint32_t, char*, char*, uint32_t);
uint32_t s_ecall_write(uint32_t, char*, char*, uint32_t);

void handle_ecall(uint64_t*, uint64_t);

#endif
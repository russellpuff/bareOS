#include <lib/ecall.h>

static inline uint64_t ecall0(uint64_t signum) {
    register uint64_t a7 asm("a7") = signum;
    register uint64_t a0 asm("a0");
    asm volatile("ecall" : "=r"(a0) : "r"(a7) : "memory");
    return a0;
}

static inline uint64_t ecall2(uint64_t signum, uint64_t x0, uint64_t x1) {
    register uint64_t a0 asm("a0") = x0;
    register uint64_t a1 asm("a1") = x1;
    register uint64_t a7 asm("a7") = signum;
    asm volatile("ecall" : "+r"(a0) : "r"(a1), "r"(a7) : "memory");
    return a0;
}

static inline uint64_t ecall3(uint64_t signum, uint64_t x0, uint64_t x1, uint64_t x2) {
    register uint64_t a0 asm("a0") = x0;
    register uint64_t a1 asm("a1") = x1;
    register uint64_t a2 asm("a2") = x2;
    register uint64_t a7 asm("a7") = signum;
    asm volatile("ecall" : "+r"(a0) : "r"(a1), "r"(a2), "r"(a7) : "memory");
    return a0;
}

uint64_t ecall_open(uint32_t device, byte* options) {
    return ecall2(ECALL_OPEN, (uint64_t)device, (uint64_t)options);
}

uint64_t ecall_close(uint32_t device, byte* options) {
    return ecall2(ECALL_CLOSE, (uint64_t)device, (uint64_t)options);
}

uint64_t ecall_read(uint32_t device, byte* options, byte* buffer) {
    return ecall3(ECALL_READ, (uint64_t)device, (uint64_t)options, (uint64_t)buffer);
}

uint64_t ecall_write(uint32_t device, byte* options, byte* buffer) {
    return ecall3(ECALL_WRITE, (uint64_t)device, (uint64_t)options, (uint64_t)buffer);
}

uint64_t ecall_spawn(char* name, char* arg) {
    return ecall2(ECALL_SPAWN, (uint64_t)name, (uint64_t)arg);
}

void ecall_pwoff(void) {
    ecall0(ECALL_PWOFF);
}

void ecall_rboot(void) {
    ecall0(ECALL_RBOOT);
}

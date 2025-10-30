#include <lib/ecall.h>

uint32_t s_ecall_open(uint32_t device, char* target) {
    (void)device;
    (void)target;
    return 0;
}

uint32_t s_ecall_close(uint32_t device, char* target) {
    (void)device;
    (void)target;
    return 0;
}

uint32_t s_ecall_read(uint32_t device, char* target, char* buffer, uint32_t length) {
    (void)device;
    (void)target;
    (void)buffer;
    (void)length;
    return 0;
}

uint32_t s_ecall_write(uint32_t device, char* target, char* buffer, uint32_t length) {
    register uint64_t a0 asm("a0") = device;
    register uint64_t a1 asm("a1") = (uint64_t)target;
    register uint64_t a2 asm("a2") = (uint64_t)buffer;
    register uint64_t a3 asm("a3") = length;
    register uint64_t a7 asm("a7") = S_ECALL_WRITE;

    asm volatile("ecall"
        : "+r"(a0)
        : "r"(a1), "r"(a2), "r"(a3), "r"(a7)
        : "memory");

    return 0;
}
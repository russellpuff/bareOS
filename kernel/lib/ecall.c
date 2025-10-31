#include <lib/ecall.h>

uint32_t ecall_open(uint32_t device, char* target) {
    (void)device;
    (void)target;
    return 0;
}

uint32_t ecall_close(uint32_t device, char* target) {
    (void)device;
    (void)target;
    return 0;
}

uint32_t ecall_read(uint32_t device, char* target, char* buffer, uint32_t length) {
    register uint64_t a0 asm("a0") = device;
    register uint64_t a1 asm("a1") = (uint64_t)target;
    register uint64_t a2 asm("a2") = (uint64_t)buffer;
    register uint64_t a3 asm("a3") = length;
    register uint64_t a7 asm("a7") = ECALL_READ;

    asm volatile("ecall"
        : "+r"(a0)
        : "r"(a1), "r"(a2), "r"(a3), "r"(a7)
        : "memory");

    return (uint32_t)a0;
}

uint32_t ecall_write(uint32_t device, char* target, char* buffer, uint32_t length) {
    register uint64_t a0 asm("a0") = device;
    register uint64_t a1 asm("a1") = (uint64_t)target;
    register uint64_t a2 asm("a2") = (uint64_t)buffer;
    register uint64_t a3 asm("a3") = length;
    register uint64_t a7 asm("a7") = ECALL_WRITE;

    asm volatile("ecall"
        : "+r"(a0)
        : "r"(a1), "r"(a2), "r"(a3), "r"(a7)
        : "memory");

    return (uint32_t)a0;
}

uint32_t ecall_spawn(char* name, char* arg) {
    register uint64_t a0 asm("a0") = (uint64_t)name;
    register uint64_t a1 asm("a1") = (uint64_t)arg;
    register uint64_t a7 asm("a7") = ECALL_SPAWN;

    asm volatile("ecall"
        : "+r"(a0)
        : "r"(a1), "r"(a7)
        : "memory");

    return (uint32_t)a0;
}

void ecall_pwoff(void) {
    register uint64_t a7 asm("a7") = ECALL_PWOFF;
    asm volatile("ecall"
        :
        : "r"(a7)
        : "memory");
}

void ecall_rboot(void) {
    register uint64_t a7 asm("a7") = ECALL_RBOOT;
    asm volatile("ecall"
        :
    : "r"(a7)
        : "memory");
}

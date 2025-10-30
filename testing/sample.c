#include <lib/uprintf.h>

__attribute__((noinline))
static void spin(uint64_t iters) {
    byte stg_num = 1;
    uint64_t counter = 1;
    const uint64_t stage = 50000000UL;
    for (uint64_t i = 1; i <= iters; ++i, ++counter) {
        asm volatile ("" ::: "memory");
        asm volatile ("nop");
        asm volatile ("nop");
        asm volatile ("nop");
        asm volatile ("nop");
        if (counter == stage) {
            printf("stage %d hit\n", stg_num);
            counter = 0;
            ++stg_num;
        }
    }
}

int main(void) {
    const uint64_t DELAY_ITERS = 250000000UL;
    printf("user task: start\n");
    spin(DELAY_ITERS);
    printf("user task: done\n");
    return 0;
}
#include <lib/io.h>

/* This file is for testing various functions that will run entirely in user mode in virtual memory */

/*
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
*/

int main(void) {
    /* Spin test: force execution to take several sections to test whether the scheduler breaks anything */
    /* Last result: everything works ok */
    /*
    const uint64_t DELAY_ITERS = 250000000UL;
    printf("user task: start\n");
    spin(DELAY_ITERS);
    printf("user task: done\n");
    */
    char line[128];
    printf("Type a line: ");
    uint32_t count = (uint32_t)gets(line, sizeof(line));
    printf("You typed (%u chars): %s\n", count, line);
    return 0;
}
#ifndef H_TIMER
#define H_TIMER

#include <barelib.h>

extern const uint64_t clint_timer_addr;
extern const uint64_t timer_interval;

void init_clk(void);
void handle_clk(void);

#endif

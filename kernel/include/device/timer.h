#ifndef H_TIMER
#define H_TIMER

#include <lib/barelib.h>

extern volatile uint64_t* clint_timer_addr;
extern const uint64_t timer_interval;

void init_clk(void);
s_interrupt handle_clk(void);

#endif

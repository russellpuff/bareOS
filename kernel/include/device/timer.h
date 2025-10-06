#ifndef H_TIMER
#define H_TIMER

#include <lib/barelib.h>

#define CLINT_TIMER_ADDR (0x2004000 + (MMU_ENABLED ? KVM_BASE : 0))
extern const uint64_t timer_interval;

void init_clk(void);
s_interrupt handle_clk(void);

#endif

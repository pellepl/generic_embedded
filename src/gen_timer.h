/*
 * gen_timer.h
 *
 *  Created on: Nov 23, 2015
 *      Author: petera
 */

#include "system.h"

#ifndef _GEN_TIMER_H_
#define _GEN_TIMER_H_

// timer maximum value
#ifndef TIMER_MAX_TICKS
#define TIMER_MAX_TICKS               ((1<<24)-1)
#endif

// do not do any operations if timer is lower than this value
#ifndef TIMER_LEAST_VALUE_BEFORE_OP
#define TIMER_LEAST_VALUE_BEFORE_OP   (16)
#endif

// while waiting for TIMER_LEAST_VALUE_BEFORE_OP, execute this instruction
#ifndef TIMER_AWAIT_SPLICE_INSTR
#define TIMER_AWAIT_SPLICE_INSTR
#endif

// enable if you have a down counting timer
//#define TIMER_DOWN_COUNTING

// tick type definition
#ifndef TIMER_TICK_TYPE
#define TIMER_TICK_TYPE u64_t
#endif

typedef TIMER_TICK_TYPE tick;

#ifndef TIMER_CALC_CURRENT_TICK
#ifndef TIMER_DOWN_COUNTING
#define TIMER_CALC_CURRENT_TICK(x)    (x)
#else  // TIMER_DOWN_COUNTING
#define TIMER_CALC_CURRENT_TICK(x)    (TIMER_MAX_TICKS - (x))
#endif // TIMER_DOWN_COUNTING
#endif // TIMER_CALC_CURRENT_TICK

typedef struct {
  tick wakeup_ticks_left;
  u32_t timer_period;
  volatile tick cur_tick;
  tick next_wakeup_tick;
} gen_tim;

void timer_init(gen_tim *tim);
void timer_set_wakeup(gen_tim *tim, tick ticks);
void timer_abort_wakeup(gen_tim *tim);
tick timer_get(gen_tim *tim);
tick timer_wakeup_time(gen_tim *tim);

// implement these
u32_t timer_hal_get_current(gen_tim *tim);
void timer_hal_set_period(gen_tim *tim, u32_t ticks);

// call this from timer overflow irq
void timer_hal_cb_overflow(gen_tim *tim);


#endif /* _GEN_TIMER_H_ */

/*
 * gen_timer.c
 *
 *  Created on: Nov 23, 2015
 *      Author: petera
 */

#include "gen_timer.h"

static inline void timer_set_period(tim, u32_t ticks) {
  timer_hal_set_period(tim, ticks);
  tim->timer_period = ticks;
}

static inline void timer_decide_next_period(gen_tim *tim) {
  if (tim->next_wakeup_tick - tim->cur_tick > TIMER_MAX_TICKS) {
    timer_set_period(tim, TIMER_MAX_TICKS);
  } else {
    timer_set_period(tim, tim->next_wakeup_tick - tim->cur_tick);
  }
}

static inline void timer_await_splice(gen_tim *tim) {
#if TIMER_LEAST_VALUE_BEFORE_OP > 0
  while (TIMER_CALC_CURRENT_TICK(timer_hal_get_current(tim)) < TIMER_LEAST_VALUE_BEFORE_OP) {
    // something is very soon to happen, do not meddle with state now
    TIMER_AWAIT_SPLICE_INSTR;
  }
#endif
}

/**
 * Set next timer callback. Must be in future, and if already set, nearer
 * current time.
 */
void timer_set_wakeup(gen_tim *tim, tick ticks) {
  timer_await_splice(tim);
  if (tim->next_wakeup_tick == 0 || ticks < tim->next_wakeup_tick) {
    tim->cur_tick += TIMER_CALC_CURRENT_TICK(timer_hal_get_current(tim));
    tim->next_wakeup_tick = ticks;
    timer_decide_next_period(tim);
  } else {
    TIM_ASSERT(0);
  }
}

/**
 * Aborts current wakeup.
 */
void timer_abort_wakeup(gen_tim *tim) {
  timer_await_splice(tim);
  if (tim->next_wakeup_tick) {
    tim->cur_tick += TIMER_CALC_CURRENT_TICK(timer_hal_get_current(tim));
    timer_set_period(tim, TIMER_MAX_TICKS);
    tim->next_wakeup_tick = 0;
  }
}

/**
 * Returns current time in ticks.
 */
tick timer_get(gen_tim *tim) {
  return TIMER_CALC_CURRENT_TICK(timer_hal_get_current(tim)) + tim->cur_tick;
}

/**
 * Returns current wakeup time in ticks.
 */
tick timer_wakeup_time(gen_tim *tim) {
  return tim->next_wakeup_tick;
}

/**
 * Call this when timer overflows. Considered to be in IRQ context.
 */
void timer_hal_cb_overflow(gen_tim *tim) {
  tim->cur_tick += tim->timer_period;
  if (tim->next_wakeup_tick == 0) {
    timer_set_period(tim, TIMER_MAX_TICKS);
  } else {
    tim->wakeup_ticks_left -= tim->timer_period;
    if (tim->wakeup_ticks_left == 0) {
      timer_set_period(tim, TIMER_MAX_TICKS);
      tim->next_wakeup_tick = 0;
      timer_fire_event(tim);
    } else {
      timer_decide_next_period(tim);
    }
  }
}

/**
 * Initialize timer
 */
void timer_init(gen_tim *tim) {
  memset(tim, 0, sizeof(gen_tim));
}

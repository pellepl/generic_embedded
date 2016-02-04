/*
 * proc_family.c
 *
 *  Created on: Feb 4, 2016
 *      Author: petera
 */

#include "system.h"
#include "arch.h"

// TODO peter fix this
void arch_busywait_us(u32_t us) {
  if (us == 0) return;
  register volatile system_counter_type *count_val =
      (volatile system_counter_type *)&(STM32_SYSTEM_TIMER->CNT);
  register system_counter_type mark = *count_val;
  register u32_t delta_ticks = us*(SYS_CPU_FREQ/1000000);
  {
    register u32_t timer_tick_cycle = SYS_CPU_FREQ/SYS_MAIN_TIMER_FREQ;
    register u32_t abs_hit = mark+delta_ticks;

    bool precede = *count_val > mark;
    if (precede) {
      while (abs_hit >= timer_tick_cycle) {
        // wait for wrap
        while (*count_val >= mark);
        // wait for hit
        while (*count_val < mark);
        delta_ticks -= timer_tick_cycle;
        abs_hit -= timer_tick_cycle;
      }
    } else {
      while (abs_hit >= timer_tick_cycle) {
        // wait for wrap
        while (*count_val < mark);
        // wait for hit
        while (*count_val >= mark);
        delta_ticks -= timer_tick_cycle;
        abs_hit -= timer_tick_cycle;
      }
    }
  }
  {
    register system_counter_type hit = mark+delta_ticks;
    if (hit < mark) {
      while (*count_val > hit);
    }
    while (*count_val < hit);
  }
}


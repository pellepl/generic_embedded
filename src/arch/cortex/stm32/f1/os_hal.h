/*
 * os_hal.h
 *
 *  Created on: Sep 23, 2013
 *      Author: petera
 */

#ifndef OS_HAL_H_
#define OS_HAL_H_

#define OS_HAL_PENDING_CTX_SWITCH \
  SCB->ICSR |= SCB_ICSR_PENDSVSET
#define OS_HAL_DISABLE_PREEMPTION \
  SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk
#define OS_HAL_ENABLE_PREEMPTION \
  SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk
#define OS_HAL_CONFIG_PREEMPTION_TICK(tick) \
  SysTick->LOAD  = ((tick) & SysTick_LOAD_RELOAD_Msk) - 1; /* set reload reg */ \
  SysTick->VAL   = 0; \
  SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk; \
  SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8);


#endif /* OS_HAL_H_ */

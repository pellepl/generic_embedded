/*
 * arch_specific.h
 *
 *  Created on: Nov 15, 2013
 *      Author: petera
 */

#ifndef ARCH_SPECIFIC_H_
#define ARCH_SPECIFIC_H_

#include "system_config.h"

#define SYS_CPU_FREQ          SystemCoreClock

#if CONFIG_STM32_SYSTEM_TIMER == 2
#define STM32_SYSTEM_TIMER          TIM2
#define STM32_SYSTEM_TIMER_IRQn     TIM2_IRQn
#define STM32_SYSTEM_TIMER_RCC      RCC_APB1Periph_TIM2
#define STM32_SYSTEM_TIMER_IRQ_FN   TIM2_IRQHandler
#define STM32_SYSTEM_TIMER_DBGMCU   DBGMCU_TIM2_STOP
#elif CONFIG_STM32_SYSTEM_TIMER == 5
#define STM32_SYSTEM_TIMER      TIM5
#define STM32_SYSTEM_TIMER_IRQn TIM5_IRQn
#define STM32_SYSTEM_TIMER_RCC  RCC_APB1Periph_TIM5
#define STM32_SYSTEM_TIMER_IRQ_FN   TIM5_IRQHandler
#define STM32_SYSTEM_TIMER_DBGMCU   DBGMCU_TIM5_STOP
#else
#error CONFIG_STM32_SYSTEM_TIMER must be defined
#endif



#endif /* ARCH_SPECIFIC_H_ */

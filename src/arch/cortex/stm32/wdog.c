/*
 * wdog.c
 *
 *  Created on: Feb 1, 2016
 *      Author: petera
 */

#include "system.h"
#include "wdog.h"
#if defined(PROC_STM32F1)
#include "stm32f10x_iwdg.h"
#endif
#if defined(PROC_STM32F4)
#include "stm32f4xx_iwdg.h"
#endif

#define LSI_FREQ 40000 // typical acc to datasheet


static void wdog_await_update(void) {
  while(IWDG_GetFlagStatus(IWDG_FLAG_RVU) == SET);
}

void WDOG_init(void) {
#if defined(PROC_STM32F1)
  DBGMCU_Config(DBGMCU_IWDG_STOP, ENABLE);
#endif
#if defined(PROC_STM32F4)
  DBGMCU_APB1PeriphConfig(DBGMCU_IWDG_STOP, ENABLE);
#endif

  IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
  IWDG_SetPrescaler(IWDG_Prescaler_256);
  IWDG_SetReload(0xfff);
  IWDG_ReloadCounter();
  IWDG_Enable();
  wdog_await_update();
}

void WDOG_start(u32_t secs) {
  u32_t lsi_ticks = secs * (LSI_FREQ / 256);

  IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
  IWDG_SetReload(MIN(0xfff, lsi_ticks));
  IWDG_ReloadCounter();
  IWDG_Enable();
  wdog_await_update();
}

void WDOG_feed(void) {
  IWDG_ReloadCounter();
}


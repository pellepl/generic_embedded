/*
 * rtc.c
 *
 *  Created on: Feb 2, 2016
 *      Author: petera
 */

#include "rtc.h"

typedef enum {
  STM32F1_RTC_CLOCK_INTERNAL = 0,
  STM32F1_RTC_CLOCK_EXTERNAL_LOW,
  STM32F1_RTC_CLOCK_EXTERNAL_HIGH
} rtc_clock;

#ifndef CONFIG_RTC_CLOCK
#define CONFIG_RTC_CLOCK STM32F1_RTC_CLOCK_EXTERNAL_LOW
#endif

void RTC_init(rtc_alarm_f alarm_callback) {
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);
  PWR_BackupAccessCmd(ENABLE);
  BKP_DeInit(); // reset bkup domain

  rtc_clock rtc_clock = CONFIG_RTC_CLOCK;
  switch (rtc_clock) {
  case STM32F1_RTC_CLOCK_INTERNAL:
    RCC_LSICmd(ENABLE);
    while(RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET);
    RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
    break;
  case STM32F1_RTC_CLOCK_EXTERNAL_LOW:
    RCC_LSEConfig(RCC_LSE_ON);
    while(RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET);
    RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
    break;
  case STM32F1_RTC_CLOCK_EXTERNAL_HIGH:
    RCC_HSEConfig(RCC_HSE_ON);
    while(RCC_GetFlagStatus(RCC_FLAG_HSERDY) == RESET);
    RCC_RTCCLKConfig(RCC_RTCCLKSource_HSE_Div128);
    break;
  }
  RCC_RTCCLKCmd(ENABLE);
  RTC_WaitForSynchro();
  RTC_WaitForLastTask();

  //RTC_ITConfig(RTC_IT_SEC, ENABLE); // second interrupt
  //RTC_WaitForLastTask();

  RTC_SetPrescaler(CONFIG_RTC_PRESCALER-1);
  RTC_WaitForLastTask();
}

u32_t RTC_get_tick(void) {
  return RTC_GetCounter();
}

void RTC_set_tick(u32_t tick) {
  RTC_SetCounter(tick);
  RTC_WaitForLastTask();
}

/*
void RCT_set_date(rtc_date *date);

void RTC_set_time(rtc_time *time);

void RTC_set_date_time(rtc_datetime *datetime);

void RTC_get_date_time(rtc_datetime *datetime);

void RTC_set_alarm(rtc_datetime *datetime);

void RTC_cancel_alarm(void);

*/


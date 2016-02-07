/*
 * rtc.c
 *
 *  Created on: Feb 2, 2016
 *      Author: petera
 */

#include "rtc.h"
#include "miniutils.h"

typedef enum {
  STM32F1_RTC_CLOCK_INTERNAL = 0,
  STM32F1_RTC_CLOCK_EXTERNAL_LOW,
  STM32F1_RTC_CLOCK_EXTERNAL_HIGH
} rtc_clock;

#ifndef CONFIG_RTC_CLOCK
#define CONFIG_RTC_CLOCK STM32F1_RTC_CLOCK_EXTERNAL_LOW
#endif

#ifndef CONFIG_STM32F1_BKP_REG_MAGIC
#define CONFIG_STM32F1_BKP_REG_MAGIC         BKP_DR2
#endif
#ifndef CONFIG_STM32F1_BKP_REG_RTCCYH
#define CONFIG_STM32F1_BKP_REG_RTCCYH        BKP_DR3
#endif
#ifndef CONFIG_STM32F1_BKP_REG_RTCCYL
#define CONFIG_STM32F1_BKP_REG_RTCCYL        BKP_DR4
#endif
#ifndef CONFIG_STM32F1_BKP_REG_RTCALH
#define CONFIG_STM32F1_BKP_REG_RTCALH        BKP_DR5
#endif
#ifndef CONFIG_STM32F1_BKP_REG_RTCALL
#define CONFIG_STM32F1_BKP_REG_RTCALL        BKP_DR6
#endif
#ifndef CONFIG_STM32F1_BKP_REG_RTCOFHH
#define CONFIG_STM32F1_BKP_REG_RTCOFHH       BKP_DR7
#endif
#ifndef CONFIG_STM32F1_BKP_REG_RTCOFHL
#define CONFIG_STM32F1_BKP_REG_RTCOFHL       BKP_DR8
#endif
#ifndef CONFIG_STM32F1_BKP_REG_RTCOFLH
#define CONFIG_STM32F1_BKP_REG_RTCOFLH       BKP_DR9
#endif
#ifndef CONFIG_STM32F1_BKP_REG_RTCOFLL
#define CONFIG_STM32F1_BKP_REG_RTCOFLL       BKP_DR10
#endif

#define MAGIC_ALARM_OFF    0x1e1f
#define MAGIC_ALARM_ON     0xd01f

#define bkprd BKP_ReadBackupRegister
#define bkpwr BKP_WriteBackupRegister

static rtc_alarm_f rtc_cb = NULL;

static u32_t rtc_bkp_get_rtc_cycles(void) {
  u16_t h = bkprd(CONFIG_STM32F1_BKP_REG_RTCCYH);
  u16_t l = bkprd(CONFIG_STM32F1_BKP_REG_RTCCYL);
  return ((u32_t)h<<16) | (u32_t)l;
}

static void rtc_bkp_set_rtc_cycles(u32_t rtccy) {
  bkpwr(CONFIG_STM32F1_BKP_REG_RTCCYH, (u16_t)(rtccy>>16));
  bkpwr(CONFIG_STM32F1_BKP_REG_RTCCYL, (u16_t)(rtccy&0xffff));
}

static bool rtc_bkp_is_alarm_enabled(void) {
  return bkprd(CONFIG_STM32F1_BKP_REG_MAGIC) == MAGIC_ALARM_ON;
}

static void rtc_bkp_set_alarm_enabled(bool enabled) {
  bkpwr(CONFIG_STM32F1_BKP_REG_MAGIC, enabled ? MAGIC_ALARM_ON : MAGIC_ALARM_OFF);
}

static u64_t rtc_bkp_get_alarm_tick(void) {
  u16_t hh = bkprd(CONFIG_STM32F1_BKP_REG_RTCALH);
  u16_t hl = bkprd(CONFIG_STM32F1_BKP_REG_RTCALL);
  return ((u64_t)hh << 48) | ((u64_t)hl << 32) | ((u64_t)RTC->ALRH<<16) | (u64_t)RTC->ALRL;
}

static void rtc_bkp_set_alarm_tick(u64_t tick) {
  bkpwr(CONFIG_STM32F1_BKP_REG_RTCALH, (u16_t)(tick>>48));
  bkpwr(CONFIG_STM32F1_BKP_REG_RTCALL, (u16_t)(tick>>32));
}

static s64_t rtc_bkp_get_offs_tick(void) {
  u16_t hh = bkprd(CONFIG_STM32F1_BKP_REG_RTCOFHH);
  u16_t hl = bkprd(CONFIG_STM32F1_BKP_REG_RTCOFHL);
  u16_t lh = bkprd(CONFIG_STM32F1_BKP_REG_RTCOFLH);
  u16_t ll = bkprd(CONFIG_STM32F1_BKP_REG_RTCOFLL);
  return ((u64_t)hh << 48) | ((u64_t)hl << 32) | ((u64_t)lh<<16) | (u64_t)ll;
}

static void rtc_bkp_set_offs_tick(s64_t tick_offs) {
  bkpwr(CONFIG_STM32F1_BKP_REG_RTCOFHH, (u16_t)(tick_offs>>48));
  bkpwr(CONFIG_STM32F1_BKP_REG_RTCOFHL, (u16_t)(tick_offs>>32));
  bkpwr(CONFIG_STM32F1_BKP_REG_RTCOFLH, (u16_t)(tick_offs>>16));
  bkpwr(CONFIG_STM32F1_BKP_REG_RTCOFLL, (u16_t)(tick_offs>>0));
}
static void rtc_handle_alarm(void) {
  if (rtc_bkp_is_alarm_enabled()) {
    // user alarm active
    // check that it is the correct cycle
    u64_t cur_tick = RTC_get_tick();
    u64_t alarm_tick = rtc_bkp_get_alarm_tick();
    if ((u32_t)(cur_tick >> 32) >= (u32_t)(alarm_tick >> 32)) {
      // correct cycle, user alarm has went off
      DBG(D_SYS, D_INFO, "RTC: alarm\n");
      bool coincide = (u32_t)alarm_tick == 0;
      if (coincide) {
        // the user alarm coincides with rtc cycle, update super cycle counter
        rtc_bkp_set_rtc_cycles(rtc_bkp_get_rtc_cycles() + 1);
      }
      rtc_bkp_set_alarm_enabled(FALSE);
      RTC_SetAlarm(0);
      RTC_WaitForLastTask();
      // blip user
      if (rtc_cb) rtc_cb();
    } else {
      // too early cycle
      rtc_bkp_set_rtc_cycles(rtc_bkp_get_rtc_cycles() + 1);
      if ((u32_t)(cur_tick >> 32) == (u32_t)(alarm_tick >> 32)) {
        // the user alarm will go off this rtc cycle, update rtc alarm
        RTC_SetAlarm(alarm_tick && 0xffffffff);
      } else {
        // the user alarm will go off some other cycle, alarm on overflow
        RTC_SetAlarm(0);
      }
      RTC_WaitForLastTask();
    }
  } else {
    // user alarm inactive, rtc cycle overflow
    rtc_bkp_set_rtc_cycles(rtc_bkp_get_rtc_cycles() + 1);
    RTC_SetAlarm(0);
    RTC_WaitForLastTask();
  }
}

void RTC_reset(void) {
  rtc_bkp_set_rtc_cycles(0);
  rtc_bkp_set_offs_tick(0);
  rtc_bkp_set_alarm_tick(0);
  rtc_bkp_set_alarm_enabled(FALSE);
  RTC_SetCounter(1);
  RTC_WaitForLastTask();
}


bool RTC_init(rtc_alarm_f alarm_callback) {
  bool res = FALSE;
  rtc_cb = alarm_callback;
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);
  PWR_BackupAccessCmd(ENABLE);
  if (bkprd(CONFIG_STM32F1_BKP_REG_MAGIC) != MAGIC_ALARM_OFF &&
      bkprd(CONFIG_STM32F1_BKP_REG_MAGIC) != MAGIC_ALARM_ON) {
    DBG(D_SYS, D_INFO, "RTC: no bkup magic\n");
    BKP_DeInit(); // reset bkup domain
    rtc_bkp_set_rtc_cycles(0);
    rtc_bkp_set_offs_tick(0);
    rtc_bkp_set_alarm_enabled(FALSE);
    bkpwr(CONFIG_STM32F1_BKP_REG_MAGIC, MAGIC_ALARM_OFF);
    res = TRUE;
  }

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

  RTC_SetAlarm(0);
  RTC_WaitForLastTask();

  RTC_SetPrescaler(CONFIG_RTC_PRESCALER-1);
  RTC_WaitForLastTask();

  if (res) {
    // to prevent false coincisions(?) @ startup we start counter at 1
    RTC_SetCounter(1);
    RTC_WaitForLastTask();
  }

  // do not use anything else than alarm interrupt as it is the only
  // one that can wake us from standby
  EXTI_InitTypeDef exti_cfg;
  EXTI_ClearITPendingBit(EXTI_Line17);
  exti_cfg.EXTI_Line = EXTI_Line17;
  exti_cfg.EXTI_Mode = EXTI_Mode_Interrupt;
  exti_cfg.EXTI_Trigger = EXTI_Trigger_Rising;
  exti_cfg.EXTI_LineCmd = ENABLE;
  EXTI_Init(&exti_cfg);

  RTC_ClearITPendingBit(RTC_IT_ALR);
  RTC_WaitForLastTask();
  RTC_ITConfig(RTC_IT_ALR, ENABLE);
  RTC_WaitForLastTask();

  return res;
}

u64_t RTC_get_tick(void) {
  u32_t cycles, counter;
  counter = RTC_GetCounter();
  cycles = rtc_bkp_get_rtc_cycles();
  if (counter > RTC_GetCounter()) {
    // amidst an overflow, reread
    counter = RTC_GetCounter();
    cycles = rtc_bkp_get_rtc_cycles();
  }
  return ((u64_t)cycles << 32) | (u64_t)counter;
}

void RTC_set_tick(u64_t tick) {
  rtc_bkp_set_rtc_cycles(tick >> 32);
  RTC_SetCounter(tick & 0xffffffff);
  RTC_WaitForLastTask();
}

void RTC_get_date_time(rtc_datetime *datetime) {
  u64_t ticks = RTC_get_tick();
  ticks += rtc_bkp_get_offs_tick();
  s64_t seconds = (s64_t)(RTC_TICK_TO_S(ticks));
  RTC_secs2datetime(seconds, datetime);
  datetime->time.millisecond = RTC_TICK_TO_MS(ticks % RTC_TICKS_PER_SEC);
}

void RTC_set_date_time(rtc_datetime *datetime) {
  DBG(D_SYS, D_INFO, "RTC set %04i-%02i-%02i %02i:%02i:%02i.%03i\n",
      datetime->date.year,
      datetime->date.month,
      datetime->date.month_day,
      datetime->time.hour,
      datetime->time.minute,
      datetime->time.second,
      datetime->time.millisecond);
  u64_t tick_now = RTC_get_tick();
  s64_t dt_secs_set = RTC_datetime2secs(datetime);
  u64_t tick_set = RTC_S_TO_TICK(dt_secs_set);
  s64_t offs = tick_set - tick_now;
  rtc_bkp_set_offs_tick(offs);
}

void RTC_set_alarm(rtc_datetime *datetime) {
  s64_t dt_secs_alarm = RTC_datetime2secs(datetime);
  s64_t ticks_alarm = RTC_S_TO_TICK(dt_secs_alarm);
  ticks_alarm -= rtc_bkp_get_offs_tick();
  RTC_set_alarm_tick(ticks_alarm);
}

void RTC_set_alarm_tick(u64_t tick) {
  DBG(D_SYS, D_INFO, "RTC set alarm:%08x%08x\n", (u32_t)(tick>>32), (u32_t)tick);
  DBG(D_SYS, D_INFO, "          now:%08x%08x\n", (u32_t)(RTC_get_tick()>>32), (u32_t)(RTC_get_tick()));
  rtc_bkp_set_alarm_tick(tick);
  RTC_SetAlarm((u32_t)(tick & 0xffffffff));
  RTC_WaitForLastTask();
  rtc_bkp_set_alarm_enabled(TRUE);
}

void RTC_cancel_alarm(void) {
  if (rtc_bkp_is_alarm_enabled()) {
    rtc_bkp_set_alarm_enabled(FALSE);
    RTC_SetAlarm(0);
    RTC_WaitForLastTask();
  }
}

void RTCAlarm_IRQHandler(void) {
  if (RTC_GetITStatus(RTC_IT_ALR) != RESET) {
    DBG(D_SYS, D_INFO, "RTC: irq\n");
    EXTI_ClearITPendingBit(EXTI_Line17);
    RTC_ClearITPendingBit(RTC_IT_ALR);
    RTC_WaitForLastTask();
    rtc_handle_alarm();
  }
}

/*
 * rtc.h
 *
 *  Created on: Feb 2, 2016
 *      Author: petera
 */

#ifndef _RTC_H_
#define _RTC_H_

#include "system.h"

#ifndef CONFIG_RTC_CLOCK_HZ
#define CONFIG_RTC_CLOCK_HZ 32768
#endif
#ifndef CONFIG_RTC_PRESCALER
 // this gives a tick resolution of 1/1024 seconds for a 32768 Hz clock
#define CONFIG_RTC_PRESCALER 32
#endif

#define RTC_TICKS_PER_SEC (CONFIG_RTC_CLOCK_HZ/CONFIG_RTC_PRESCALER)

#define RTC_TICK_TO_MS(t)   (((t)*1000)/RTC_TICKS_PER_SEC)
#define RTC_MS_TO_TICK(ms)  (((ms)*RTC_TICKS_PER_SEC)/1000)
#define RTC_TICK_TO_S(t)    ((t)/RTC_TICKS_PER_SEC)
#define RTC_S_TO_TICK(s)    ((s)*RTC_TICKS_PER_SEC)

typedef struct {
  u8_t hour;
  u8_t minute;
  u8_t second;
  u16_t millisecond;
} rtc_time;

typedef struct {
  u16_t year;
  u8_t month;
  u8_t year_day;
  u8_t month_day;
  u8_t week_day;
} rtc_date;

typedef struct {
  rtc_date date;
  rtc_time time;
} rtc_datetime;

typedef void (*rtc_alarm_f)(void);

u32_t RTC_month2secs(u8_t month, bool is_leap);
s64_t RTC_year2secs(s64_t year, bool *is_leap);
u64_t RTC_datetime2secs(const rtc_datetime *tm);
int RTC_secs2datetime(s64_t t, rtc_datetime *tm);

void RTC_init(rtc_alarm_f alarm_callback);
u64_t RTC_get_tick(void);
void RTC_set_tick(u64_t tick);
/**
 * It is not assured that any set alarm will be recalculated if
 * set by date time.
 * To be sure, cancel alarm and set it again.
 */
void RTC_set_date_time(rtc_datetime *datetime);
void RTC_get_date_time(rtc_datetime *datetime);
void RTC_set_alarm(rtc_datetime *datetime);
void RTC_set_alarm_tick(u64_t tick);
void RTC_cancel_alarm(void);
void RTC_reset(void);


#endif /* _RTC_H_ */

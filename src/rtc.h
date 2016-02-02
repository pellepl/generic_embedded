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
#define CONFIG_RTC_PRESCALER 32
#endif

typedef struct {
  u8_t hour;
  u8_t day;
  u8_t second;
  u32_t tick;
} rtc_time;

typedef struct {
  u16_t year;
  u8_t month;
  u8_t day;
} rtc_date;

typedef struct {
  rtc_date date;
  rtc_time time;
} rtc_datetime;

typedef void (*rtc_alarm_f)(rtc_datetime datetime);

void RTC_init(rtc_alarm_f alarm_callback);
u32_t RTC_get_tick(void);
void RTC_set_tick(u32_t tick);
void RTC_set_date(rtc_date *date);
void RTC_set_time(rtc_time *time);
void RTC_set_date_time(rtc_datetime *datetime);
void RTC_get_date_time(rtc_datetime *datetime);
void RTC_set_alarm(rtc_datetime *datetime);
u32_t RTC_get_ticks(void);
void RTC_cancel_alarm(void);


#endif /* _RTC_H_ */

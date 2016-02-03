/*
 * rtc_common.c
 *
 *  Created on: Feb 3, 2016
 *      Author: petera
 */

#include "system.h"
#include "rtc.h"

//
// Kudos:
// taken from http://git.musl-libc.org/cgit/musl/tree/src/time/
//

/* 2000-03-01 (mod 400 year, immediately after feb29 */
#define LEAPOCH (946684800LL + 86400*(31+29))

#define DAYS_PER_400Y (365*400 + 97)
#define DAYS_PER_100Y (365*100 + 24)
#define DAYS_PER_4Y   (365*4   + 1)

int RTC_secs2datetime(s64_t t, rtc_datetime *tm) {
  s64_t days, secs;
  s32_t remdays, remsecs, remyears;
  s32_t qc_cycles, c_cycles, q_cycles;
  s32_t years, months;
  s32_t wday, yday, leap;
  static const char days_in_month[] = {31,30,31,30,31,31,30,31,30,31,31,29};

  /* Reject time_t values whose year would overflow int */
  if (t < S32_MIN * 31622400LL || t > S32_MAX * 31622400LL)
    return -1;

  secs = t - LEAPOCH;
  days = secs / 86400;
  remsecs = secs % 86400;
  if (remsecs < 0) {
    remsecs += 86400;
    days--;
  }

  wday = (3+days)%7;
  if (wday < 0) wday += 7;

  qc_cycles = days / DAYS_PER_400Y;
  remdays = days % DAYS_PER_400Y;
  if (remdays < 0) {
    remdays += DAYS_PER_400Y;
    qc_cycles--;
  }

  c_cycles = remdays / DAYS_PER_100Y;
  if (c_cycles == 4) c_cycles--;
  remdays -= c_cycles * DAYS_PER_100Y;

  q_cycles = remdays / DAYS_PER_4Y;
  if (q_cycles == 25) q_cycles--;
  remdays -= q_cycles * DAYS_PER_4Y;

  remyears = remdays / 365;
  if (remyears == 4) remyears--;
  remdays -= remyears * 365;

  leap = !remyears && (q_cycles || !c_cycles);
  yday = remdays + 31 + 28 + leap;
  if (yday >= 365+leap) yday -= 365+leap;

  years = remyears + 4*q_cycles + 100*c_cycles + 400*qc_cycles;

  for (months=0; days_in_month[months] <= remdays; months++)
    remdays -= days_in_month[months];

  if (years+100 > S32_MAX || years+100 < S32_MIN)
    return -1;

  tm->date.year = years + 100;
  tm->date.month = months + 2;
  if (tm->date.month >= 12) {
    tm->date.month -=12;
    tm->date.year++;
  }
  tm->date.month_day = remdays + 1;
  tm->date.week_day = wday;
  tm->date.year_day = yday;

  tm->time.hour = remsecs / 3600;
  tm->time.minute = remsecs / 60 % 60;
  tm->time.second = remsecs % 60;
  tm->time.millisecond = 0;

  return 0;
}

u64_t RTC_datetime2secs(const rtc_datetime *tm)
{
  bool is_leap;
  s64_t year = tm->date.year;
  s32_t month = tm->date.month;
  if (month >= 12 || month < 0) {
    int adj = month / 12;
    month %= 12;
    if (month < 0) {
      adj--;
      month += 12;
    }
    year += adj;
  }
  u64_t t = RTC_year2secs(year, &is_leap);
  t += RTC_month2secs(month, is_leap);
  t += 86400LL * (tm->date.month_day-1);
  t += 3600LL * tm->time.hour;
  t += 60LL * tm->time.minute;
  t += tm->time.second;
  return t;
}

s64_t RTC_year2secs(s64_t year, bool *is_leap)
{
  if (year-2ULL <= 136) {
    s32_t y = year;
    s32_t leaps = (y-68)>>2;
    if (!((y-68)&3)) {
      leaps--;
      if (is_leap) *is_leap = TRUE;
    } else if (is_leap) *is_leap = FALSE;
    return 31536000*(y-70) + 86400*leaps;
  }

  s32_t cycles, centuries, leaps, rem;

  cycles = (year-100) / 400;
  rem = (year-100) % 400;
  if (rem < 0) {
    cycles--;
    rem += 400;
  }
  if (!rem) {
    if (is_leap) *is_leap = TRUE;
    centuries = 0;
    leaps = 0;
  } else {
    if (rem >= 200) {
      if (rem >= 300) centuries = 3, rem -= 300;
      else centuries = 2, rem -= 200;
    } else {
      if (rem >= 100) centuries = 1, rem -= 100;
      else centuries = 0;
    }
    if (!rem) {
      if (is_leap) *is_leap = FALSE;
      leaps = 0;
    } else {
      leaps = rem / 4U;
      rem %= 4U;
      if (is_leap) *is_leap = !rem;
    }
  }

  leaps += 97*cycles + 24*centuries - *is_leap;

  return (year-100) * 31536000LL + leaps * 86400LL + 946684800 + 86400;
}

u32_t RTC_month2secs(u8_t month, bool is_leap)
{
  static const u32_t secs_through_month[] = {
    0, 31*86400, 59*86400, 90*86400,
    120*86400, 151*86400, 181*86400, 212*86400,
    243*86400, 273*86400, 304*86400, 334*86400 };
  u32_t t = secs_through_month[month];
  if (is_leap && month >= 2) t+=86400;
  return t;
}

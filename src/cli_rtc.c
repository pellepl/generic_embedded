#include "cli.h"
#include "miniutils.h"
#include "rtc.h"

static void parse_date(rtc_datetime *dt, u32_t argc, u32_t y, u32_t mo, u32_t d, u32_t h, u32_t mi, u32_t s) {
  dt->time.millisecond = 0;
  if (argc == 1) {
    dt->time.second = y;
  } else if (argc == 2) {
    dt->time.minute = y;
    dt->time.second = mo;
  } else if (argc == 3) {
    dt->time.hour = y;
    dt->time.minute = mo;
    dt->time.second = d;
  } else if (argc == 4) {
    dt->date.month_day = y;
    dt->time.hour = mo;
    dt->time.minute = d;
    dt->time.second = h;
  } else if (argc == 5) {
    dt->date.month = y-1;
    dt->date.month_day = mo;
    dt->time.hour = d;
    dt->time.minute = h;
    dt->time.second = mi;
  } else if (argc == 6) {
    dt->date.year = y;
    dt->date.month = mo-1;
    dt->date.month_day = d;
    dt->time.hour = h;
    dt->time.minute = mi;
    dt->time.second = s;
  }
}

static int cli_rtc_init(u32_t argc) {
  RTC_init(NULL);
  return CLI_OK;
}

static int cli_rtc_tick(u32_t argc, u32_t tick1, u32_t tick2) {
  if (argc == 0) {
    u64_t t = RTC_get_tick();
    print("RTC tick:0x%08x%08x\n", (u32_t)(t>>32), (u32_t)(t&0xffffffff));
  } else if (argc==1) {
    RTC_set_tick(tick1);
  } else if (argc==2) {
    RTC_set_tick(((u64_t)tick1 << 32) | (u64_t)tick2);
  } else {
    return CLI_ERR_PARAM;
  }
  return CLI_OK;
}

static int cli_rtc_alarm_tick(u32_t argc, u32_t tick1, u32_t tick2) {
  if (argc==1) {
    RTC_set_alarm_tick(tick1);
  } else if (argc==2) {
    RTC_set_alarm_tick(((u64_t)tick1 << 32) | (u64_t)tick2);
  } else {
    return CLI_ERR_PARAM;
  }
  return CLI_OK;
}

static int cli_rtc_alarm(u32_t argc, u32_t y, u32_t mo, u32_t d, u32_t h, u32_t mi, u32_t s) {
  rtc_datetime dt;
  RTC_get_date_time(&dt);
  dt.time.millisecond = 0;
  if (argc >= 1 && argc <= 6) {
    parse_date(&dt, argc, y, mo, d, h,mi, s);
  } else {
    return CLI_ERR_PARAM;
  }
  RTC_set_alarm(&dt);
  return CLI_OK;
}

static int cli_rtc_time(u32_t argc, u32_t y, u32_t mo, u32_t d, u32_t h, u32_t mi, u32_t s) {
  rtc_datetime dt;
  RTC_get_date_time(&dt);
  if (argc == 0) {
    print("date: %04i-%02i-%02i time: %02i:%02i:%02i.%03i\n",
          dt.date.year,
          dt.date.month+1,
          dt.date.month_day,
          dt.time.hour,
          dt.time.minute,
          dt.time.second,
          dt.time.millisecond
          );
  } else {
    dt.time.millisecond = 0;
    if (argc >= 1 && argc <= 6) {
      parse_date(&dt, argc, y, mo, d, h,mi, s);
    } else {
      return CLI_ERR_PARAM;
    }
    RTC_set_date_time(&dt);
  }

  return CLI_OK;
}

static int cli_rtc_reset(u32_t argc, u32_t seconds) {
  RTC_reset();
  return CLI_OK;
}


CLI_MENU_START(rtc)
CLI_FUNC("alarm", cli_rtc_alarm, "Set alarm in datetime\n"
         "alarm (((((<year>) <month>) <day>) <hour>) <minute>) <sec>\n")
 CLI_FUNC("alarm_tick", cli_rtc_alarm_tick, "Set alarm in ticks\n"
          "alarm_tick (<set_value_hi> <set_value_lo>)\n")
CLI_FUNC("init", cli_rtc_init, "Init RTC")
CLI_FUNC("reset", cli_rtc_reset, "Reset RTC")
CLI_FUNC("tick", cli_rtc_tick, "Get/set RTC ticks\n"
    "tick (<set_value_hi> <set_value_lo>)\n")
CLI_FUNC("time", cli_rtc_time, "Get/set RTC time\n"
    "time ((((((<year>) <month>) <day>) <hour>) <minute>) <sec>)\n")
CLI_MENU_END

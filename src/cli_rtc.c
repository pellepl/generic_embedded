#include "cli.h"
#include "miniutils.h"
#include "rtc.h"

static int cli_rtc_init(u32_t argc) {
  RTC_init(NULL);
  return CLI_OK;
}

static int cli_rtc_tick(u32_t argc, u32_t tick) {
  if (argc == 0) {
    print("RTC tick:%i\n", RTC_get_tick());
  } else {
    RTC_set_tick(tick);
  }
  return CLI_OK;
}

CLI_MENU_START(rtc)
CLI_FUNC("init", cli_rtc_init, "Init RTC")
CLI_FUNC("tick", cli_rtc_tick, "Get/set RTC ticks\n"
    "tick (<set_value>)\n")
CLI_MENU_END

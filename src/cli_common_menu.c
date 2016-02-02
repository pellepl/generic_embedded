/*
 * cli_common_menu.c
 *
 *  Created on: Feb 1, 2016
 *      Author: petera
 */

#include "system.h"
#include "cli.h"

#ifndef CONFIG_CLI_SYS_OFF
CLI_EXTERN_MENU(system)
#endif
#if defined(CONFIG_GPIO) && !defined(CONFIG_CLI_GPIO_OFF)
CLI_EXTERN_MENU(gpio)
#endif
#if defined(CONFIG_RTC) && !defined(CONFIG_CLI_RTC_OFF)
CLI_EXTERN_MENU(rtc)
#endif
#ifndef CONFIG_CLI_BUS_OFF
CLI_EXTERN_MENU(bus)
#endif
#ifndef CONFIG_CLI_DEV_OFF
CLI_EXTERN_MENU(dev)
#endif

CLI_MENU_START(common)

#ifndef CONFIG_CLI_BUS_OFF
CLI_SUBMENU(bus, "bus", "SUBMENU: processor busses")
#endif
#ifndef CONFIG_CLI_DEV_OFF
CLI_SUBMENU(dev, "dev", "SUBMENU: peripheral devices")
#endif
#if defined(CONFIG_GPIO) && !defined(CONFIG_CLI_GPIO_OFF)
CLI_SUBMENU(gpio, "gpio", "SUBMENU: GPIO")
#endif
#if defined(CONFIG_RTC) && !defined(CONFIG_CLI_RTC_OFF)
CLI_SUBMENU(rtc, "rtc", "SUBMENU: RTC")
#endif
#ifndef CONFIG_CLI_SYS_OFF
CLI_SUBMENU(system, "sys", "SUBMENU: system")
#endif

CLI_MENU_END


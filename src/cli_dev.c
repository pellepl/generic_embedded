/*
 * cli_dev.c
 *
 *  Created on: Jan 31, 2016
 *      Author: petera
 */

#include "cli.h"

#if defined(CONFIG_ADXL345) && !defined(CONFIG_CLI_ADXL345_OFF)
CLI_EXTERN_MENU(adxl345)
#endif
#if defined(CONFIG_HMC5883L) && !defined(CONFIG_CLI_HMC5883L_OFF)
CLI_EXTERN_MENU(hmc5883l)
#endif
#if defined(CONFIG_M24M01) && !defined(CONFIG_CLI_M24M01_OFF)
CLI_EXTERN_MENU(m24m01)
#endif
#if defined(CONFIG_ITG3200) && !defined(CONFIG_CLI_ITG3200_OFF)
CLI_EXTERN_MENU(itg3200)
#endif




CLI_MENU_START(dev)

#if defined(CONFIG_ADXL345) && !defined(CONFIG_CLI_ADXL345_OFF)
CLI_SUBMENU(adxl345, "adxl", "SUBMENU: ADXL345 accelerometer")
#endif

#if defined(CONFIG_HMC5883L) && !defined(CONFIG_CLI_HMC5883L_OFF)
CLI_SUBMENU(hmc5883l, "hmc", "SUBMENU: HMC5883L magnetometer")
#endif

#if defined(CONFIG_ITG3200) && !defined(CONFIG_CLI_ITG3200_OFF)
CLI_SUBMENU(itg3200, "itg", "SUBMENU: ITG3200 gyroscope")
#endif

#if defined(CONFIG_M24M01) && !defined(CONFIG_CLI_M24M01_OFF)
CLI_SUBMENU(m24m01, "m24", "SUBMENU: M24M01 EEPROM")
#endif


CLI_MENU_END

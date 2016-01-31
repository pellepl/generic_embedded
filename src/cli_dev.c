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




CLI_MENU_START(dev)

#if defined(CONFIG_ADXL345) && !defined(CONFIG_CLI_ADXL345_OFF)
CLI_SUBMENU(adxl345, "adxl", "ADXL345 accelerometer submenu")
#endif

#if defined(CONFIG_HMC5883L) && !defined(CONFIG_CLI_HMC5883L_OFF)
CLI_SUBMENU(hmc5883l, "hmc", "HMC5883L magnetometer submenu")
#endif

CLI_MENU_END

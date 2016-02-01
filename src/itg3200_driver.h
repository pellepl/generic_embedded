/*
 * itg3200_driver.h
 *
 * See https://www.sparkfun.com/datasheets/Sensors/Gyro/PS-ITG-3200-00-01.4.pdf
 *
 *  Created on: Jan 2, 2016
 *      Author: petera
 */

#ifndef ITG3200_DRIVER_H_
#define ITG3200_DRIVER_H_

#include "system.h"
#include "i2c_dev.h"

#define I2C_ERR_ITG3200_BUSY     -1500
#define I2C_ERR_ITG3200_ID       -1501
#define I2C_ERR_ITG3200_STATE    -1502
#define I2C_ERR_ITG3200_NULLPTR  -1503

#define ITG3200_R_WHOAMI          0x00
#define ITG3200_R_SMPLRT_DIV      0x15
#define ITG3200_R_DLPF_FS         0x16
#define ITG3200_R_INT_CFG         0x17
#define ITG3200_R_INT_STATUS      0x1a
#define ITG3200_R_TEMP_OUT_H      0x1b
#define ITG3200_R_TEMP_OUT_L      0x1c
#define ITG3200_R_GYRO_XOUT_H     0x1d
#define ITG3200_R_GYRO_XOUT_L     0x1e
#define ITG3200_R_GYRO_YOUT_H     0x1f
#define ITG3200_R_GYRO_YOUT_L     0x20
#define ITG3200_R_GYRO_ZOUT_H     0x21
#define ITG3200_R_GYRO_ZOUT_L     0x22
#define ITG3200_R_PWR_MGM         0x3e


#define ITG3200_ID                0b01101000

#define ITG3200_I2C_ADDR          0b11010000
#define ITG3200_I2C_ADDR0         (ITG3200_I2C_ADDR | 0b00)
#define ITG3200_I2C_ADDR1         (ITG3200_I2C_ADDR | 0b10)

typedef struct {
  s16_t temp,x,y,z;
} itg_reading;

typedef enum {
  ITG3200_STATE_IDLE = 0,
  ITG3200_STATE_READ,
  ITG3200_STATE_READ_INTERRUPTS,
  ITG3200_STATE_ID,
  ITG3200_STATE_CONFIG,
} itg_state;

typedef enum {
  ITG3200_LP_256  = 0,
  ITG3200_LP_188  = 1,
  ITG3200_LP_98  = 2,
  ITG3200_LP_42  = 3,
  ITG3200_LP_20  = 4,
  ITG3200_LP_10  = 5,
  ITG3200_LP_5  = 6,
} itg_lp_cfg_rate;

typedef enum {
  ITG3200_INT_ACTIVE_HI = 0,
  ITG3200_INT_ACTIVE_LO = 1,
} itg_int_cfg_act;
typedef enum {
  ITG3200_INT_PUSHPULL = 0,
  ITG3200_INT_OPENDRAIN = 1,
} itg_int_cfg_odpp;
typedef enum {
  ITG3200_INT_50US_PULSE = 0,
  ITG3200_INT_LATCH = 1,
} itg_int_cfg_latch_pulse;
typedef enum {
  ITG3200_INT_CLR_SR = 0,
  ITG3200_INT_CLR_ANY= 1,
} itg_int_cfg_clr;
typedef enum {
  ITG3200_INT_NO_PLL = 0,
  ITG3200_INT_WHEN_PLL = 1,
} itg_int_cfg_int_when_pll_ready;
typedef enum {
  ITG3200_INT_NO_DATA = 0,
  ITG3200_INT_WHEN_DATA = 1,
} itg_int_cfg_int_when_data_ready;

typedef enum {
  ITG3200_CLK_INTERNAL = 0,
  ITG3200_CLK_PLL_X = 1,
  ITG3200_CLK_PLL_Y = 2,
  ITG3200_CLK_PLL_Z = 3,
  ITG3200_CLK_PLL_EXT_32K = 4,
  ITG3200_CLK_PLL_EXT_19_2M = 5,
} itg_pwr_cfg_clk;
typedef enum {
  ITG3200_NO_RESET = 0,
  ITG3200_RESET = 1
} itg_pwr_cfg_reset;
typedef enum {
  ITG3200_ACTIVE = 0,
  ITG3200_LOW_POWER = 1,
} itg_pwr_cfg_sleep;
typedef enum {
  ITG3200_STNDBY_X_OFF = 0,
  ITG3200_STNDBY_X_ON = 1,
} itg_pwr_cfg_stndby_x;
typedef enum {
  ITG3200_STNDBY_Y_OFF = 0,
  ITG3200_STNDBY_Y_ON = 1,
} itg_pwr_cfg_stndby_y;
typedef enum {
  ITG3200_STNDBY_Z_OFF = 0,
  ITG3200_STNDBY_Z_ON = 1,
} itg_pwr_cfg_stndby_z;

typedef struct {
  itg_pwr_cfg_clk pwr_clk;
  itg_pwr_cfg_reset pwr_reset;
  itg_pwr_cfg_sleep pwr_sleep;
  itg_pwr_cfg_stndby_x pwr_stdby_x;
  itg_pwr_cfg_stndby_y pwr_stdby_y;
  itg_pwr_cfg_stndby_z pwr_stdby_z;
  u8_t samplerate_div;
  itg_lp_cfg_rate lp_filter_rate;
  itg_int_cfg_act int_act;
  itg_int_cfg_odpp int_odpp;
  itg_int_cfg_latch_pulse int_latch_pulse;
  itg_int_cfg_clr int_clr;
  itg_int_cfg_int_when_pll_ready int_pll;
  itg_int_cfg_int_when_data_ready int_data;
} itg_cfg;

typedef struct {
  union {
    __attribute (( packed )) struct {
      u8_t data_ready : 1;
      u8_t _rsv0 : 1;
      u8_t pll_ready : 1;
      u8_t _rsv1 : 5;
    };
    u8_t raw;
  };
} itg_int_status;

typedef struct itg3200_dev_s {
  i2c_dev i2c_dev;
  itg_state state;
  void (* callback)(struct itg3200_dev_s *dev, itg_state state, int res);
  i2c_dev_sequence seq[4];
  union {
    bool *id_ok;
    itg_reading *data;
    itg_int_status *int_status;
  } arg;
  u8_t tmp_buf[10];
} itg3200_dev;

void itg_open(itg3200_dev *dev, i2c_bus *bus, bool ad0, u32_t clock, void (*itg_callback)(itg3200_dev *dev, itg_state state, int res));
void itg_close(itg3200_dev *dev);
int itg_check_id(itg3200_dev *dev, bool *id_ok);
int itg_config(itg3200_dev *dev, itg_cfg *cfg);
int itg_read_data(itg3200_dev *dev, itg_reading *data);
int itg_read_interrupts(itg3200_dev *dev, itg_int_status *int_src);

#endif /* ITG3200_DRIVER_H_ */

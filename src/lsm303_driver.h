/*
 * lsm303_driver.h
 *
 *  Created on: Aug 24, 2013
 *      Author: petera
 */

#ifndef LSM303_DRIVER_H_
#define LSM303_DRIVER_H_

#include "i2c_dev.h"

#define I2C_ERR_LSM303_STATE    -1000
#define I2C_ERR_LSM303_BAD_READ -1001

enum lsm_state {
  LSM_STATE_IDLE = 0,
  LSM_STATE_INIT,
  LSM_STATE_READ_ACC,
  LSM_STATE_READ_MAG,
  LSM_STATE_READ_BOTH,
  LSM_STATE_ERROR
};

typedef struct lsm303_dev_s {
  volatile enum lsm_state state;
  i2c_dev i2c_acc;
  i2c_dev i2c_mag;
  void (*callback)(struct lsm303_dev_s *dev, int res);
  u8_t buf[6];
  i2c_dev_sequence seq[2];
  s16_t acc[3];
  s16_t mag[3];
  s16_t mag_limit_min[3];
  s16_t mag_limit_max[3];
  u8_t lp_filter_acc;
  u8_t lp_filter_mag;
} lsm303_dev;

void lsm_open(lsm303_dev *dev, i2c_bus *bus, bool sa0, void (*lsm_callback)(lsm303_dev *dev, int res));
void lsm_close(lsm303_dev *dev);
int lsm_config_default(lsm303_dev *dev);
void lsm_set_mag_limits(lsm303_dev *dev, s16_t min[3], s16_t max[3]);
int lsm_read_acc(lsm303_dev *dev);
int lsm_read_mag(lsm303_dev *dev);
int lsm_read_both(lsm303_dev *dev);
s16_t *lsm_get_acc_reading(lsm303_dev *dev);
s16_t *lsm_get_mag_reading(lsm303_dev *dev);
u16_t lsm_get_heading(lsm303_dev *dev);
void lsm_set_lowpass(lsm303_dev *dev, u8_t acc, u8_t mag);

#endif /* LSM303_DRIVER_H_ */

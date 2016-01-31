/*
 * hmc5883l_driver.h
 *
 *  Created on: Dec 29, 2015
 *      Author: petera
 */

#ifndef HMC5883L_DRIVER_H_
#define HMC5883L_DRIVER_H_

#include "i2c_dev.h"
#include "hmc5883l_hw.h"

#define I2C_ERR_HMC5883L_BUSY     -1400
#define I2C_ERR_HMC5883L_ID       -1401
#define I2C_ERR_HMC5883L_STATE    -1402
#define I2C_ERR_HMC5883L_NULLPTR  -1403

// HMC5883L_R_CFG_A_MA
typedef enum {
  hmc5883l_samples_avg_1 = 0b00,
  hmc5883l_samples_avg_2 = 0b01,
  hmc5883l_samples_avg_4 = 0b10,
  hmc5883l_samples_avg_8 = 0b11,
} hmc5883l_samples_avg;

// HMC5883L_R_CFG_A_DO
typedef enum {
  hmc5883l_data_output_0_75 = 0b000,
  hmc5883l_data_output_1_5  = 0b001,
  hmc5883l_data_output_3    = 0b010,
  hmc5883l_data_output_7_5  = 0b011,
  hmc5883l_data_output_15   = 0b100,
  hmc5883l_data_output_35   = 0b101,
  hmc5883l_data_output_75   = 0b110,
} hmc5883l_data_output;

// HMC5883L_R_CFG_A_MS
typedef enum {
  hmc5883l_measurement_mode_normal   = 0b00,
  hmc5883l_measurement_mode_positive = 0b01,
  hmc5883l_measurement_mode_negative = 0b10,
} hmc5883l_measurement_mode;

// HMC5883L_R_CFG_B_GN
typedef enum {
  hmc5883l_gain_0_88 = 0b000,
  hmc5883l_gain_1_3  = 0b001,
  hmc5883l_gain_1_9  = 0b010,
  hmc5883l_gain_2_5  = 0b011,
  hmc5883l_gain_4_0  = 0b100,
  hmc5883l_gain_4_7  = 0b101,
  hmc5883l_gain_5_6  = 0b110,
  hmc5883l_gain_8_1  = 0b111,
} hmc5883l_gain;

// HMC5883L_R_MODE_HS
typedef enum {
  hmc5883l_i2c_speed_normal  = 0b0,
  hmc5883l_i2c_speed_3400khz = 0b1,
} hmc5883l_i2c_speed;

// HMC5883L_R_MODE_MD
typedef enum {
  hmc5883l_mode_continuous = 0b00,
  hmc5883l_mode_single     = 0b01,
  hmc5883l_mode_idle       = 0b10,
} hmc5883l_mode;

typedef struct {
  s16_t x,y,z;
} hmc_reading;

typedef enum {
  HMC5883L_STATE_IDLE = 0,
  HMC5883L_STATE_CONFIG,
  HMC5883L_STATE_READ,
  HMC5883L_STATE_READ_DRDY,
  HMC5883L_STATE_ID,
} hmc_state;

typedef struct hmc5883l_dev_s {
  i2c_dev i2c_dev;
  hmc_state state;
  void (* callback)(struct hmc5883l_dev_s *dev, hmc_state state, int res);
  i2c_dev_sequence seq[3];
  union {
    bool *drdy;
    bool *id_ok;
    hmc_reading *data;
  } arg;
  u8_t tmp_buf[8];
} hmc5883l_dev;

void hmc_open(hmc5883l_dev *dev, i2c_bus *bus, u32_t clock, void (*hmc_callback)(hmc5883l_dev *dev, hmc_state state, int res));
void hmc_close(hmc5883l_dev *dev);
int hmc_check_id(hmc5883l_dev *dev, bool *id_ok);
int hmc_config_default(hmc5883l_dev *dev);
int hmc_config(hmc5883l_dev *dev,
    hmc5883l_mode mode,
    hmc5883l_i2c_speed speed,
    hmc5883l_gain gain,
    hmc5883l_measurement_mode meas_mode,
    hmc5883l_data_output output,
    hmc5883l_samples_avg avg);
int hmc_read(hmc5883l_dev *dev, hmc_reading *data);
int hmc_drdy(hmc5883l_dev *dev, bool *drdy);

#endif /* HMC5883L_DRIVER_H_ */

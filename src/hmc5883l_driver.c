/*
 * hmc5883l_driver.c
 *
 *  Created on: Dec 29, 2015
 *      Author: petera
 */

#include "hmc5883l_driver.h"
#include "hmc5883l_hw.h"

static void hmc_cb(i2c_dev *idev, int res) {
  hmc5883l_dev *dev = (hmc5883l_dev *)I2C_DEV_get_user_data(idev);
  hmc_state old_state = dev->state;
  if (res != I2C_OK) {
    dev->state = HMC5883L_STATE_IDLE;
    if (dev->callback) dev->callback(dev, old_state, res);
    return;
  }

  switch (dev->state) {
  case HMC5883L_STATE_CONFIG:
    break;

  case HMC5883L_STATE_READ:
    if (dev->arg.data) {
      dev->arg.data->x = (dev->tmp_buf[1] << 8) | dev->tmp_buf[2];
      dev->arg.data->y = (dev->tmp_buf[3] << 8) | dev->tmp_buf[4];
      dev->arg.data->z = (dev->tmp_buf[5] << 8) | dev->tmp_buf[6];
    }
    break;

  case HMC5883L_STATE_READ_DRDY:
    if (dev->arg.drdy) {
      *dev->arg.drdy = dev->tmp_buf[1] & HMC5883L_R_SR_RDY_MASK;
    }
    break;

  case HMC5883L_STATE_ID:
    if (dev->arg.id_ok) {
      *dev->arg.id_ok =
          dev->tmp_buf[1] == HMC5883L_ID_A &&
          dev->tmp_buf[2] == HMC5883L_ID_B &&
          dev->tmp_buf[3] == HMC5883L_ID_C;
    }
    break;

  case HMC5883L_STATE_IDLE:
    res = I2C_ERR_HMC5883L_STATE;
    break;
  }

  dev->state = HMC5883L_STATE_IDLE;

  if (dev->callback) dev->callback(dev, old_state, res);
}

void hmc_open(hmc5883l_dev *dev, i2c_bus *bus, u32_t clock,
    void (*hmc_callback)(hmc5883l_dev *dev, hmc_state state, int res)) {
  memset(dev, 0, sizeof(hmc5883l_dev));
  I2C_DEV_init(&dev->i2c_dev, clock, bus, HMC5883L_I2C_ADDR);
  I2C_DEV_set_user_data(&dev->i2c_dev, dev);
  I2C_DEV_set_callback(&dev->i2c_dev, hmc_cb);
  I2C_DEV_open(&dev->i2c_dev);
  dev->callback = hmc_callback;
}

void hmc_close(hmc5883l_dev *dev) {
  I2C_DEV_close(&dev->i2c_dev);
}

int hmc_config(hmc5883l_dev *dev,
    hmc5883l_mode mode,
    hmc5883l_i2c_speed speed,
    hmc5883l_gain gain,
    hmc5883l_measurement_mode meas_mode,
    hmc5883l_data_output output,
    hmc5883l_samples_avg avg) {
  u8_t r_cfg_a =
      ((avg << HMC5883L_R_CFG_A_MA_POS) & HMC5883L_R_CFG_A_MA_MASK) |
      ((output << HMC5883L_R_CFG_A_DO_POS) & HMC5883L_R_CFG_A_DO_MASK) |
      ((meas_mode << HMC5883L_R_CFG_A_MS_POS) & HMC5883L_R_CFG_A_MS_MASK);
  u8_t r_cfg_b =
      ((gain << HMC5883L_R_CFG_B_GN_POS) & HMC5883L_R_CFG_B_GN_MASK);
  u8_t r_mode =
      ((speed << HMC5883L_R_MODE_HS_POS) & HMC5883L_R_MODE_HS_MASK) |
      ((mode << HMC5883L_R_MODE_MD_POS) & HMC5883L_R_MODE_MD_MASK);

  dev->tmp_buf[0] = HMC5883L_R_CFG_A;
  dev->tmp_buf[1] = r_cfg_a;
  dev->tmp_buf[2] = HMC5883L_R_CFG_B;
  dev->tmp_buf[3] = r_cfg_b;
  dev->tmp_buf[4] = HMC5883L_R_MODE;
  dev->tmp_buf[5] = r_mode;

  I2C_SEQ_TX_STOP_C(dev->seq[0], &dev->tmp_buf[0], 2);
  I2C_SEQ_TX_STOP_C(dev->seq[1], &dev->tmp_buf[2], 2);
  I2C_SEQ_TX_STOP_C(dev->seq[2], &dev->tmp_buf[4], 2);

  dev->state = HMC5883L_STATE_CONFIG;
  int res = I2C_DEV_sequence(&dev->i2c_dev, &dev->seq[0], 3);
  if (res != I2C_OK) {
    dev->state = HMC5883L_STATE_IDLE;
  }

  return res;
}

int hmc_read(hmc5883l_dev *dev, hmc_reading *data) {
  if (data == NULL) {
    return I2C_ERR_HMC5883L_NULLPTR;
  }

  dev->arg.data = data;
  dev->tmp_buf[0] = HMC5883L_R_X_MSB;
  I2C_SEQ_TX_C(dev->seq[0], &dev->tmp_buf[0], 1);
  I2C_SEQ_RX_STOP_C(dev->seq[1], &dev->tmp_buf[1], 6);
  dev->state = HMC5883L_STATE_READ;
  int res = I2C_DEV_sequence(&dev->i2c_dev, &dev->seq[0], 2);
  if (res != I2C_OK) {
    dev->state = HMC5883L_STATE_IDLE;
  }

  return res;
}

int hmc_check_id(hmc5883l_dev *dev, bool *id_ok) {
  if (id_ok == NULL) {
    return I2C_ERR_HMC5883L_NULLPTR;
  }

  dev->arg.id_ok = id_ok;
  dev->tmp_buf[0] = HMC5883L_R_ID_A;
  I2C_SEQ_TX_C(dev->seq[0], &dev->tmp_buf[0], 1);
  I2C_SEQ_RX_STOP_C(dev->seq[1], &dev->tmp_buf[1], 3);
  dev->state = HMC5883L_STATE_ID;
  int res = I2C_DEV_sequence(&dev->i2c_dev, &dev->seq[0], 2);
  if (res != I2C_OK) {
    dev->state = HMC5883L_STATE_IDLE;
  }

  return res;
}

int hmc_drdy(hmc5883l_dev *dev, bool *drdy) {
  if (drdy == NULL) {
    return I2C_ERR_HMC5883L_NULLPTR;
  }

  dev->arg.drdy = drdy;
  dev->tmp_buf[0] = HMC5883L_R_SR;
  I2C_SEQ_TX_C(dev->seq[0], &dev->tmp_buf[0], 1);
  I2C_SEQ_RX_STOP_C(dev->seq[1], &dev->tmp_buf[1], 1);
  dev->state = HMC5883L_STATE_READ_DRDY;
  int res = I2C_DEV_sequence(&dev->i2c_dev, &dev->seq[0], 2);
  if (res != I2C_OK) {
    dev->state = HMC5883L_STATE_IDLE;
  }

  return res;
}



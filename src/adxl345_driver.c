/*
 * adxl345_driver.c
 *
 *  Created on: Jan 10, 2016
 *      Author: petera
 */

#include "adxl345_driver.h"


static void adxl_cb(i2c_dev *idev, int res) {
  adxl345_dev *dev = (adxl345_dev *)I2C_DEV_get_user_data(idev);
  adxl_state old_state = dev->state;
  if (res != I2C_OK) {
    dev->state = ADXL345_STATE_IDLE;
    if (dev->callback) dev->callback(dev, old_state, res);
    return;
  }

  switch (dev->state) {
  case ADXL345_STATE_CONFIG_POWER:
  case ADXL345_STATE_SET_OFFSET:
  case ADXL345_STATE_CONFIG_TAP:
  case ADXL345_STATE_CONFIG_ACTIVITY:
  case ADXL345_STATE_CONFIG_FREEFALL:
  case ADXL345_STATE_CONFIG_INTERRUPTS:
  case ADXL345_STATE_CONFIG_FORMAT:
  case ADXL345_STATE_CONFIG_FIFO:
    break;

  case ADXL345_STATE_READ:
    if (dev->arg.data) {
      dev->arg.data->x = (dev->tmp_buf[2] << 8) | dev->tmp_buf[1];
      dev->arg.data->y = (dev->tmp_buf[4] << 8) | dev->tmp_buf[3];
      dev->arg.data->z = (dev->tmp_buf[6] << 8) | dev->tmp_buf[5];
    }
    break;

  case ADXL345_STATE_READ_INTERRUPTS:
    if (dev->arg.int_src) {
      *dev->arg.int_src = dev->tmp_buf[1];
    }
    break;

  case ADXL345_STATE_READ_FIFO:
    if (dev->arg.fifo_status) {
      dev->arg.fifo_status->raw = dev->tmp_buf[1];
    }
    break;

  case ADXL345_STATE_READ_ACT_TAP:
    if (dev->arg.act_tap_status) {
      dev->arg.act_tap_status->raw = dev->tmp_buf[1];
    }
    break;

  case ADXL345_STATE_READ_ALL_STATUS:
    if (dev->arg.status) {
      dev->arg.status->int_src = dev->tmp_buf[3];
      dev->arg.status->act_tap_status.raw = dev->tmp_buf[4];
      dev->arg.status->fifo_status.raw = dev->tmp_buf[5];
    }
    break;

  case ADXL345_STATE_ID:
    if (dev->arg.id_ok) {
      *dev->arg.id_ok =
          dev->tmp_buf[1] == ADXL345_ID;
    }
    break;

  case ADXL345_STATE_IDLE:
  default:
    res = I2C_ERR_ADXL345_STATE;
    break;
  }

  dev->state = ADXL345_STATE_IDLE;

  if (dev->callback) dev->callback(dev, old_state, res);
}

void adxl_open(adxl345_dev *dev, i2c_bus *bus, u32_t clock,
    void (*adxl_callback)(adxl345_dev *dev, adxl_state state, int res)) {
  memset(dev, 0, sizeof(adxl345_dev));
  I2C_DEV_init(&dev->i2c_dev, clock, bus, ADXL345_I2C_ADDR);
  I2C_DEV_set_user_data(&dev->i2c_dev, dev);
  I2C_DEV_set_callback(&dev->i2c_dev, adxl_cb);
  I2C_DEV_open(&dev->i2c_dev);
  dev->callback = adxl_callback;
}

void adxl_close(adxl345_dev *dev) {
  I2C_DEV_close(&dev->i2c_dev);
}


int adxl_check_id(adxl345_dev *dev, bool *id_ok) {
  if (id_ok == NULL) {
    return I2C_ERR_ADXL345_NULLPTR;
  }

  dev->arg.id_ok = id_ok;
  dev->tmp_buf[0] = ADXL345_R_DEVID;
  I2C_SEQ_TX_C(dev->seq[0], &dev->tmp_buf[0], 1);
  I2C_SEQ_RX_STOP_C(dev->seq[1], &dev->tmp_buf[1], 1);
  dev->state = ADXL345_STATE_ID;
  int res = I2C_DEV_sequence(&dev->i2c_dev, &dev->seq[0], 2);
  if (res != I2C_OK) {
    dev->state = ADXL345_STATE_IDLE;
  }

  return res;
}

int adxl_config_power(adxl345_dev *dev,
    bool low_power, adxl_rate rate,
    bool link, bool auto_sleep, adxl_mode mode,
    adxl_sleep sleep) {
  // ADXL345_R_BW_RATE 0x2c low_power, rate
  // ADXL345_R_POWER_CTL 0x2d link, auto_sleep, mode
  dev->tmp_buf[0] = ADXL345_R_BW_RATE;
  dev->tmp_buf[1] = 0 |
      (low_power ? (1<<4) : 0) |
      rate;
  dev->tmp_buf[2] = 0 |
      (link ? (1<<5) : 0) |
      (auto_sleep ? (1<<4) : 0) |
      (mode<<3) |
      sleep;

  I2C_SEQ_TX_STOP_C(dev->seq[0], &dev->tmp_buf[0], 3);
  dev->state = ADXL345_STATE_CONFIG_POWER;
  int res = I2C_DEV_sequence(&dev->i2c_dev, &dev->seq[0], 1);
  if (res != I2C_OK) {
    dev->state = ADXL345_STATE_IDLE;
  }

  return res;
}

int adxl_set_offset(adxl345_dev *dev, s8_t x, s8_t y, s8_t z) {
  // ADXL345_R_OFSX 0x1e x
  // ADXL345_R_OFSY 0x1f y
  // ADXL345_R_OFSZ 0x20 z
  dev->tmp_buf[0] = ADXL345_R_OFSX;
  dev->tmp_buf[1] = x;
  dev->tmp_buf[2] = y;
  dev->tmp_buf[3] = z;

  I2C_SEQ_TX_STOP_C(dev->seq[0], &dev->tmp_buf[0], 4);
  dev->state = ADXL345_STATE_SET_OFFSET;
  int res = I2C_DEV_sequence(&dev->i2c_dev, &dev->seq[0], 1);
  if (res != I2C_OK) {
    dev->state = ADXL345_STATE_IDLE;
  }

  return res;
}

int adxl_config_tap(adxl345_dev *dev,
    adxl_axes tap_ena, u8_t thresh, u8_t dur, u8_t latent, u8_t window, bool suppress) {
  // ADXL345_R_TAP_AXES 0x2a suppress, tap_ena
  // ADXL345_R_THRESH_TAP 0x1d thresh
  // ADXL345_R_DUR 0x21 dur
  // ADXL345_R_LATENT 0x22 latent
  // ADXL345_R_WINDOW 0x23 window
  dev->tmp_buf[0] = ADXL345_R_TAP_AXES;
  dev->tmp_buf[1] = 0 |
      (suppress ? (1<<3) : 0) |
      tap_ena;
  dev->tmp_buf[2] = ADXL345_R_THRESH_TAP;
  dev->tmp_buf[3] = thresh;
  dev->tmp_buf[4] = ADXL345_R_DUR;
  dev->tmp_buf[5] = dur;
  dev->tmp_buf[6] = latent;
  dev->tmp_buf[7] = window;

  I2C_SEQ_TX_STOP_C(dev->seq[0], &dev->tmp_buf[0], 2);
  I2C_SEQ_TX_STOP_C(dev->seq[1], &dev->tmp_buf[2], 2);
  I2C_SEQ_TX_STOP_C(dev->seq[2], &dev->tmp_buf[4], 4);
  dev->state = ADXL345_STATE_CONFIG_TAP;
  int res = I2C_DEV_sequence(&dev->i2c_dev, &dev->seq[0], 3);
  if (res != I2C_OK) {
    dev->state = ADXL345_STATE_IDLE;
  }

  return res;
}

int adxl_config_activity(adxl345_dev *dev,
    adxl_acdc ac_dc, adxl_axes act_ena, adxl_axes inact_ena, u8_t thr_act, u8_t thr_inact, u8_t time_inact) {
  // ADXL345_R_THRESH_ACT 0x24 thr_act
  // ADXL345_R_THRESH_INACT 0x25 thr_inact
  // ADXL345_R_TIME_INACT 0x26 time_inact
  // ADXL345_R_ACT_INACT_CTL 0x27 ac_dc, act_ena, inact_ena
  dev->tmp_buf[0] = ADXL345_R_THRESH_ACT;
  dev->tmp_buf[1] = thr_act;
  dev->tmp_buf[2] = thr_inact;
  dev->tmp_buf[3] = time_inact;
  dev->tmp_buf[4] = 0 |
      (ac_dc<<7) |
      (act_ena<<4) |
      (ac_dc<<3) |
      (inact_ena);

  I2C_SEQ_TX_STOP_C(dev->seq[0], &dev->tmp_buf[0], 5);
  dev->state = ADXL345_STATE_CONFIG_ACTIVITY;
  int res = I2C_DEV_sequence(&dev->i2c_dev, &dev->seq[0], 1);
  if (res != I2C_OK) {
    dev->state = ADXL345_STATE_IDLE;
  }

  return res;
}

int adxl_config_freefall(adxl345_dev *dev, u8_t thresh, u8_t time) {
  // ADXL345_R_THRESH_FF 0x28 thresh
  // ADXL345_R_TIME_FF 0x29 time
  dev->tmp_buf[0] = ADXL345_R_THRESH_FF;
  dev->tmp_buf[1] = thresh;
  dev->tmp_buf[2] = time;

  I2C_SEQ_TX_STOP_C(dev->seq[0], &dev->tmp_buf[0], 3);
  dev->state = ADXL345_STATE_CONFIG_FREEFALL;
  int res = I2C_DEV_sequence(&dev->i2c_dev, &dev->seq[0], 1);
  if (res != I2C_OK) {
    dev->state = ADXL345_STATE_IDLE;
  }

  return res;
}

int adxl_config_interrupts(adxl345_dev *dev, u8_t int_ena, u8_t int_map) {
  // ADXL345_R_INT_ENABLE 0x2e int_ena
  // ADXL345_R_INT_MAP 0x2f int_map
  dev->tmp_buf[0] = ADXL345_R_INT_ENABLE;
  dev->tmp_buf[1] = int_ena;
  dev->tmp_buf[2] = int_map;

  I2C_SEQ_TX_STOP_C(dev->seq[0], &dev->tmp_buf[0], 3);
  dev->state = ADXL345_STATE_CONFIG_INTERRUPTS;
  int res = I2C_DEV_sequence(&dev->i2c_dev, &dev->seq[0], 1);
  if (res != I2C_OK) {
    dev->state = ADXL345_STATE_IDLE;
  }

  return res;
}

int adxl_config_format(adxl345_dev *dev,
    bool int_inv, bool full_res, bool justify, adxl_range range) {
  // ADXL345_R_DATA_FORMAT 0x31 int_inv, full_res, justify, range
  dev->tmp_buf[0] = ADXL345_R_DATA_FORMAT;
  dev->tmp_buf[1] = 0 |
      (int_inv ? (1<<5) : 0) |
      (full_res ? (1<<3) : 0) |
      (justify ? (1<<2): 0) |
      range;

  I2C_SEQ_TX_STOP_C(dev->seq[0], &dev->tmp_buf[0], 2);
  dev->state = ADXL345_STATE_CONFIG_FORMAT;
  int res = I2C_DEV_sequence(&dev->i2c_dev, &dev->seq[0], 1);
  if (res != I2C_OK) {
    dev->state = ADXL345_STATE_IDLE;
  }

  return res;
}

int adxl_config_fifo(adxl345_dev *dev,
    adxl_fifo_mode mode, adxl_pin_int trigger, u8_t samples) {
  // ADXL345_R_FIFO_CTL 0x38 mode, trigger, samples
  dev->tmp_buf[0] = ADXL345_R_FIFO_CTL;
  dev->tmp_buf[1] = 0 |
      (mode ? (1<<6) : 0) |
      (trigger ? (1<<5) : 0) |
      (samples & 0x1f);

  I2C_SEQ_TX_STOP_C(dev->seq[0], &dev->tmp_buf[0], 2);
  dev->state = ADXL345_STATE_CONFIG_FIFO;
  int res = I2C_DEV_sequence(&dev->i2c_dev, &dev->seq[0], 1);
  if (res != I2C_OK) {
    dev->state = ADXL345_STATE_IDLE;
  }

  return res;
}

int adxl_read_data(adxl345_dev *dev, adxl_reading *data) {
  if (data == NULL) {
    return I2C_ERR_ADXL345_NULLPTR;
  }

  dev->arg.data = data;
  dev->tmp_buf[0] = ADXL345_R_DATAX0;
  I2C_SEQ_TX_C(dev->seq[0], &dev->tmp_buf[0], 1);
  I2C_SEQ_RX_STOP_C(dev->seq[1], &dev->tmp_buf[1], 6);
  dev->state = ADXL345_STATE_READ;
  int res = I2C_DEV_sequence(&dev->i2c_dev, &dev->seq[0], 2);
  if (res != I2C_OK) {
    dev->state = ADXL345_STATE_IDLE;
  }

  return res;
}

int adxl_read_interrupts(adxl345_dev *dev, u8_t *int_src) {
  if (int_src == NULL) {
    return I2C_ERR_ADXL345_NULLPTR;
  }

  dev->arg.int_src = int_src;
  dev->tmp_buf[0] = ADXL345_R_INT_SOURCE;
  I2C_SEQ_TX_C(dev->seq[0], &dev->tmp_buf[0], 1);
  I2C_SEQ_RX_STOP_C(dev->seq[1], &dev->tmp_buf[1], 1);
  dev->state = ADXL345_STATE_READ_INTERRUPTS;
  int res = I2C_DEV_sequence(&dev->i2c_dev, &dev->seq[0], 2);
  if (res != I2C_OK) {
    dev->state = ADXL345_STATE_IDLE;
  }

  return res;
}

int adxl_read_fifo_status(adxl345_dev *dev, adxl_fifo_status *fifo_status) {
  if (fifo_status == NULL) {
    return I2C_ERR_ADXL345_NULLPTR;
  }

  dev->arg.fifo_status = fifo_status;
  dev->tmp_buf[0] = ADXL345_R_FIFO_STATUS;
  I2C_SEQ_TX_C(dev->seq[0], &dev->tmp_buf[0], 1);
  I2C_SEQ_RX_STOP_C(dev->seq[1], &dev->tmp_buf[1], 1);
  dev->state = ADXL345_STATE_READ_FIFO;
  int res = I2C_DEV_sequence(&dev->i2c_dev, &dev->seq[0], 2);
  if (res != I2C_OK) {
    dev->state = ADXL345_STATE_IDLE;
  }

  return res;
}

int adxl_read_act_tap_status(adxl345_dev *dev, adxl_act_tap_status *act_tap_status) {
  if (act_tap_status == NULL) {
    return I2C_ERR_ADXL345_NULLPTR;
  }

  dev->arg.act_tap_status = act_tap_status;
  dev->tmp_buf[0] = ADXL345_R_ACT_TAP_STATUS;
  I2C_SEQ_TX_C(dev->seq[0], &dev->tmp_buf[0], 1);
  I2C_SEQ_RX_STOP_C(dev->seq[1], &dev->tmp_buf[1], 1);
  dev->state = ADXL345_STATE_READ_ACT_TAP;
  int res = I2C_DEV_sequence(&dev->i2c_dev, &dev->seq[0], 2);
  if (res != I2C_OK) {
    dev->state = ADXL345_STATE_IDLE;
  }

  return res;
}


int adxl_read_status(adxl345_dev *dev, adxl_status *status) {
  if (status == NULL) {
    return I2C_ERR_ADXL345_NULLPTR;
  }

  dev->arg.status = status;
  dev->tmp_buf[0] = ADXL345_R_INT_SOURCE;
  dev->tmp_buf[1] = ADXL345_R_ACT_TAP_STATUS;
  dev->tmp_buf[2] = ADXL345_R_FIFO_STATUS;
  I2C_SEQ_TX_C(dev->seq[0], &dev->tmp_buf[0], 1);
  I2C_SEQ_RX_STOP_C(dev->seq[1], &dev->tmp_buf[3], 1);
  I2C_SEQ_TX_C(dev->seq[2], &dev->tmp_buf[1], 1);
  I2C_SEQ_RX_STOP_C(dev->seq[3], &dev->tmp_buf[4], 1);
  I2C_SEQ_TX_C(dev->seq[4], &dev->tmp_buf[2], 1);
  I2C_SEQ_RX_STOP_C(dev->seq[5], &dev->tmp_buf[5], 1);
  dev->state = ADXL345_STATE_READ_ALL_STATUS;
  int res = I2C_DEV_sequence(&dev->i2c_dev, &dev->seq[0], 6);
  if (res != I2C_OK) {
    dev->state = ADXL345_STATE_IDLE;
  }

  return res;
}

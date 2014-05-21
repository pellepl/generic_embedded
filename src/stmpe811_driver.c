/*
 * stmpe811_driver.c
 *
 *  Created on: Feb 28, 2014
 *      Author: petera
 */

#include "stmpe811_driver.h"
#include "stmpe811_driver_hw.h"

#define STMPE_FUNC_ENTRY \
  int res = I2C_OK; \
  if (dev->state != STMPE_ST_IDLE && dev->state != STMPE_ST_ERROR) { \
    return I2C_ERR_DEV_BUSY;\
  }

#define STMPE_FUNC_EXIT \
  if (res != I2C_OK) { \
    dev->state = STMPE_ST_ERROR; \
  }\
  return res


// i2c callback state switcher

static void _stmpe811_cb(i2c_dev *idev, int res) {
  stmpe811_dev *dev = (stmpe811_dev *)I2C_DEV_get_user_data(idev);
  if (dev->state == STMPE_ST_IDLE || dev->state == STMPE_ST_ERROR) {
    return;
  }
  stmpe811_state_e st = dev->state;
  if (res != I2C_OK) {
    dev->state = STMPE_ST_ERROR;
    if (dev->callback) dev->callback(dev, res, st, 0);
    return;
  }
  u16_t data = 0;
  u8_t *buf = dev->buf;
  switch (dev->state) {
  case STMPE_ST_IDENTIFY:
    data = (buf[0] << 8) | (buf[1]);
    if (data != 0x0811) {
      res = I2C_ERR_STMPE811_ID;
    }
    break;

  case STMPE_ST_SOFT_RESET:
    dev->buf[1] &= STMPE_SYS_CTRL1_SOFT_RESET_MASK;
    dev->buf[1] |= STMPE_SYS_CTRL1_SOFT_RESET_ON;
    I2C_SEQ_TX_STOP_C(dev->seq[0], &dev->buf[0], 2);
    dev->state = STMPE_ST_SOFT_RESET1;
    res = I2C_DEV_sequence(&dev->i2c_stmpe, &dev->seq[0], 1);
    if (res == I2C_OK) return;
    break;
  case STMPE_ST_SOFT_RESET1:
    dev->buf[1] &= STMPE_SYS_CTRL1_SOFT_RESET_MASK;
    dev->buf[1] |= STMPE_SYS_CTRL1_SOFT_RESET_OFF;
    I2C_SEQ_TX_STOP_C(dev->seq[0], &dev->buf[0], 2);
    dev->state = STMPE_ST_SOFT_RESET2;
    res = I2C_DEV_sequence(&dev->i2c_stmpe, &dev->seq[0], 1);
    if (res == I2C_OK) return;
    break;
  case STMPE_ST_SOFT_RESET2:
    break;

  case STMPE_ST_HIBERNATE:
    dev->buf[1] &= STMPE_SYS_CTRL1_HIBERNATE_MASK;
    dev->buf[1] |= dev->arg ? STMPE_SYS_CTRL1_HIBERNATE_ON : STMPE_SYS_CTRL1_HIBERNATE_OFF;
    I2C_SEQ_TX_STOP_C(dev->seq[0], &dev->buf[0], 2);
    dev->state = STMPE_ST_HIBERNATE1;
    res = I2C_DEV_sequence(&dev->i2c_stmpe, &dev->seq[0], 1);
    if (res == I2C_OK) return;
    break;
  case STMPE_ST_HIBERNATE1:
    break;

  case STMPE_ST_CTRL_POW_BLOCK:
    dev->buf[1] &= ~((dev->arg >> 8) | (dev->arg & 0xff)); // remove config for disable & enable mask
    dev->buf[1] |= (dev->arg & 0xff); // 1 = disable, so set these for disable mask
    I2C_SEQ_TX_STOP_C(dev->seq[0], &dev->buf[0], 2);
    dev->state = STMPE_ST_CTRL_POW_BLOCK1;
    res = I2C_DEV_sequence(&dev->i2c_stmpe, &dev->seq[0], 1);
    if (res == I2C_OK) return;
    break;
  case STMPE_ST_CTRL_POW_BLOCK1:
    break;

  case STMPE_ST_CTRL_CONFIG_PORT_AF:
    break;

  case STMPE_ST_INT_CONFIG:
    dev->buf[1] &= ~(STMPE_INT_CTRL_POLARITY_MASK | STMPE_INT_CTRL_TYPE_MASK);
    dev->buf[1] |= dev->arg;
    I2C_SEQ_TX_STOP_C(dev->seq[0], &dev->buf[0], 2);
    dev->state = STMPE_ST_INT_CONFIG1;
    res = I2C_DEV_sequence(&dev->i2c_stmpe, &dev->seq[0], 1);
    if (res == I2C_OK) return;
    break;
  case STMPE_ST_INT_CONFIG1:
    break;

  case STMPE_ST_INT_ENABLE:
    dev->buf[1] &= ~(STMPE_INT_CTRL_GLOBAL_MASK);
    dev->buf[1] |= dev->arg;
    I2C_SEQ_TX_STOP_C(dev->seq[0], &dev->buf[0], 2);
    dev->state = STMPE_ST_INT_ENABLE1;
    res = I2C_DEV_sequence(&dev->i2c_stmpe, &dev->seq[0], 1);
    if (res == I2C_OK) return;
    break;
  case STMPE_ST_INT_ENABLE1:
    break;

  case STMPE_ST_INT_MASK:
    break;

  case STMPE_ST_INT_STATUS:
    data = buf[0];
    break;

  case STMPE_ST_INT_CLEAR:
    break;

  case STMPE_ST_INT_GPIO_MASK:
    break;

  case STMPE_ST_INT_GPIO_STATUS:
    data = buf[0];
    break;

  case STMPE_ST_INT_GPIO_CLEAR:
    break;

  case STMPE_ST_INT_ADC_MASK:
    break;

  case STMPE_ST_INT_ADC_STATUS:
    data = buf[0];
    break;

  case STMPE_ST_INT_ADC_CLEAR:
    break;

  case STMPE_ST_GPIO_CFG_DETECT:
    break;

  case STMPE_ST_GPIO_CFG_DIR:
    break;

  case STMPE_ST_GPIO_STATUS:
    data = buf[0];
    break;

  case STMPE_ST_GPIO_DETECT_STATUS:
    data = buf[0];
    break;

  case STMPE_ST_GPIO_SET:
    break;

  case STMPE_ST_GPIO_CLR:
    break;

  case STMPE_ST_ADC_CONF:
    break;

  case STMPE_ST_ADC_TRIG:
    break;

  case STMPE_ST_ADC_STAT:
    data = buf[0];
    break;

  case STMPE_ST_ADC_READ:
    data = (buf[0] << 8) | (buf[1]);
    break;

  case STMPE_ST_TEMP_CFG:
    dev->buf[1] &= ~(STMPE_TEMP_CTRL_ENABLE_MASK | STMPE_TEMP_CTRL_ACQ_MOD_MASK);
    dev->buf[1] |= dev->arg;
    I2C_SEQ_TX_STOP_C(dev->seq[0], &dev->buf[0], 2);
    dev->state = STMPE_ST_TEMP_CFG1;
    res = I2C_DEV_sequence(&dev->i2c_stmpe, &dev->seq[0], 1);
    if (res == I2C_OK) return;
    break;
  case STMPE_ST_TEMP_CFG1:
    break;

  case STMPE_ST_TEMP_CFG_TH:
    dev->buf[1] &= ~(STMPE_TEMP_CTRL_THRES_RANGE_MASK | STMPE_TEMP_CTRL_THRES_EN_MASK);
    dev->buf[1] |= dev->buf[2];
    dev->buf[3] = STMPE_REG_TEMP_TH;
    dev->buf[4] = (dev->arg >> 8);
    dev->buf[5] = (dev->arg & 0xff);
    I2C_SEQ_TX_STOP_C(dev->seq[0], &dev->buf[0], 2);
    I2C_SEQ_TX_STOP_C(dev->seq[1], &dev->buf[3], 3);
    dev->state = STMPE_ST_TEMP_CFG_TH1;
    res = I2C_DEV_sequence(&dev->i2c_stmpe, &dev->seq[0], 2);
    if (res == I2C_OK) return;
    break;
  case STMPE_ST_TEMP_CFG_TH1:
    break;

  case STMPE_ST_TEMP_ACQ:
    dev->buf[1] &= ~(STMPE_TEMP_CTRL_ACQ_MASK);
    dev->buf[1] |= dev->arg;
    I2C_SEQ_TX_STOP_C(dev->seq[0], &dev->buf[0], 2);
    dev->state = STMPE_ST_TEMP_ACQ1;
    res = I2C_DEV_sequence(&dev->i2c_stmpe, &dev->seq[0], 1);
    if (res == I2C_OK) return;
    break;
  case STMPE_ST_TEMP_ACQ1:
    break;

  case STMPE_ST_TEMP_READ:
    data = (dev->buf[0] << 8) | (dev->buf[1]);
    break;

  default:
    res = I2C_ERR_STMPE811_UNKNOWN_STATE;
    data = st;
    break;
  }

  dev->state = res == I2C_OK ? STMPE_ST_IDLE : STMPE_ST_ERROR;
  if (dev->callback) dev->callback(dev, res, st, data);
}

// api functions

void stmpe811_open(stmpe811_dev *dev, i2c_bus *bus,  void (*callback)(struct stmpe811_dev_s *dev, int res,
    stmpe811_state_e state, u16_t arg)) {
  const u32_t clock = 100000;
  I2C_DEV_init(&dev->i2c_stmpe, clock, bus, 0x82);
  I2C_DEV_set_user_data(&dev->i2c_stmpe, dev);
  I2C_DEV_set_callback(&dev->i2c_stmpe, _stmpe811_cb);
  I2C_DEV_open(&dev->i2c_stmpe);

  dev->callback = callback;
  dev->state = STMPE_ST_IDLE;
}
void stmpe811_close(stmpe811_dev *dev) {
  I2C_DEV_close(&dev->i2c_stmpe);
}

// read out
int stmpe811_identify(stmpe811_dev *dev) {
  STMPE_FUNC_ENTRY;
  dev->state = STMPE_ST_IDENTIFY;
  dev->buf[0] = STMPE_REG_CHIP_ID;
  I2C_SEQ_TX_C(dev->seq[0], &dev->buf[0], 1);
  I2C_SEQ_RX_STOP_C(dev->seq[1], &dev->buf[0], 2);
  res = I2C_DEV_sequence(&dev->i2c_stmpe, &dev->seq[0], 2);
  STMPE_FUNC_EXIT;
}

// read out, write reset bit to one, write reset bit to zero
int stmpe811_ctrl_reset_soft(stmpe811_dev *dev) {
  STMPE_FUNC_ENTRY;
  dev->state = STMPE_ST_SOFT_RESET;
  dev->buf[0] = STMPE_REG_SYS_CTRL1;
  I2C_SEQ_TX_C(dev->seq[0], &dev->buf[0], 1);
  I2C_SEQ_RX_STOP_C(dev->seq[1], &dev->buf[1], 1);
  res = I2C_DEV_sequence(&dev->i2c_stmpe, &dev->seq[0], 2);
  STMPE_FUNC_EXIT;
}

// read out, write hibernate bit to given value
int stmpe811_ctrl_hibernate(stmpe811_dev *dev, bool hibernate) {
  STMPE_FUNC_ENTRY;
  dev->state = STMPE_ST_HIBERNATE;
  dev->buf[0] = STMPE_REG_SYS_CTRL1;
  dev->arg = hibernate;
  I2C_SEQ_TX_C(dev->seq[0], &dev->buf[0], 1);
  I2C_SEQ_RX_STOP_C(dev->seq[1], &dev->buf[1], 1);
  res = I2C_DEV_sequence(&dev->i2c_stmpe, &dev->seq[0], 2);
  STMPE_FUNC_EXIT;
}

// read out, write modified bits denoted by both enable and disable masks
int stmpe811_ctrl_power_block(stmpe811_dev *dev, u8_t block_enable_mask, u8_t block_disable_mask) {
  STMPE_FUNC_ENTRY;
  dev->state = STMPE_ST_CTRL_POW_BLOCK;
  dev->buf[0] = STMPE_REG_SYS_CTRL2;
  block_disable_mask &=
      (STMPE_SYS_CTRL2_TS_MASK | STMPE_SYS_CTRL2_GPIO_MASK | STMPE_SYS_CTRL2_TSC_MASK | STMPE_SYS_CTRL2_ADC_MASK);
  block_enable_mask &=
      (STMPE_SYS_CTRL2_TS_MASK | STMPE_SYS_CTRL2_GPIO_MASK | STMPE_SYS_CTRL2_TSC_MASK | STMPE_SYS_CTRL2_ADC_MASK);
  dev->arg = (block_enable_mask << 8) | block_disable_mask ;
  I2C_SEQ_TX_C(dev->seq[0], &dev->buf[0], 1);
  I2C_SEQ_RX_STOP_C(dev->seq[1], &dev->buf[1], 1);
  res = I2C_DEV_sequence(&dev->i2c_stmpe, &dev->seq[0], 2);
  STMPE_FUNC_EXIT;
}

// write
int stmpe811_ctrl_config_port_af(stmpe811_dev *dev, u8_t gpio_analog_digital_mask) {
  STMPE_FUNC_ENTRY;
  dev->state = STMPE_ST_CTRL_CONFIG_PORT_AF;
  dev->buf[0] = STMPE_REG_GPIO_AF;
  dev->buf[1] = gpio_analog_digital_mask;
  I2C_SEQ_TX_STOP_C(dev->seq[0], &dev->buf[0], 2);
  res = I2C_DEV_sequence(&dev->i2c_stmpe, &dev->seq[0], 1);
  STMPE_FUNC_EXIT;
}

// read out, write modified config bits
int stmpe811_int_config(stmpe811_dev *dev, stmpe_int_cfg_pol_e polarity, stmpe_int_cfg_typ_e type) {
  STMPE_FUNC_ENTRY;
  dev->state = STMPE_ST_INT_CONFIG;
  dev->buf[0] = STMPE_REG_INT_CTRL;
  u8_t cfg = 0;
  switch (polarity) {
  case STMPE_INT_POLARITY_ACTIVE_LOW_FALLING:
    cfg |= STMPE_INT_CTRL_POLARITY_FALLING;
    break;
  case STMPE_INT_POLARITY_ACTIVE_HIGH_RISING:
    cfg |= STMPE_INT_CTRL_POLARITY_RISING;
    break;
  }
  switch (type) {
  case STMPE_INT_TYPE_EDGE:
    cfg |= STMPE_INT_CTRL_TYPE_EDGE;
    break;
  case STMPE_INT_TYPE_LEVEL:
    cfg |= STMPE_INT_CTRL_TYPE_LEVEL;
    break;
  }
  dev->arg = cfg;
  I2C_SEQ_TX_C(dev->seq[0], &dev->buf[0], 1);
  I2C_SEQ_RX_STOP_C(dev->seq[1], &dev->buf[1], 1);
  res = I2C_DEV_sequence(&dev->i2c_stmpe, &dev->seq[0], 2);
  STMPE_FUNC_EXIT;
}

// read out, write modified config bits
int stmpe811_int_enable(stmpe811_dev *dev) {
  STMPE_FUNC_ENTRY;
  dev->state = STMPE_ST_INT_ENABLE;
  dev->buf[0] = STMPE_REG_INT_CTRL;
  dev->arg = STMPE_INT_CTRL_GLOBAL_ENABLE;
  I2C_SEQ_TX_C(dev->seq[0], &dev->buf[0], 1);
  I2C_SEQ_RX_STOP_C(dev->seq[1], &dev->buf[1], 1);
  res = I2C_DEV_sequence(&dev->i2c_stmpe, &dev->seq[0], 2);
  STMPE_FUNC_EXIT;
}

// read out, write modified config bits
int stmpe811_int_disable(stmpe811_dev *dev) {
  STMPE_FUNC_ENTRY;
  dev->state = STMPE_ST_INT_ENABLE;
  dev->buf[0] = STMPE_REG_INT_CTRL;
  dev->arg = STMPE_INT_CTRL_GLOBAL_DISABLE;
  I2C_SEQ_TX_C(dev->seq[0], &dev->buf[0], 1);
  I2C_SEQ_RX_STOP_C(dev->seq[1], &dev->buf[1], 1);
  res = I2C_DEV_sequence(&dev->i2c_stmpe, &dev->seq[0], 2);
  STMPE_FUNC_EXIT;
}

// write
int stmpe811_int_mask(stmpe811_dev *dev, u8_t int_mask) {
  STMPE_FUNC_ENTRY;
  dev->state = STMPE_ST_INT_MASK;
  dev->buf[0] = STMPE_REG_INT_EN;
  dev->buf[1] = int_mask;
  I2C_SEQ_TX_STOP_C(dev->seq[0], &dev->buf[0], 2);
  res = I2C_DEV_sequence(&dev->i2c_stmpe, &dev->seq[0], 1);
  STMPE_FUNC_EXIT;
}

// read out
int stmpe811_int_status(stmpe811_dev *dev) {
  STMPE_FUNC_ENTRY;
  dev->state = STMPE_ST_INT_STATUS;
  dev->buf[0] = STMPE_REG_INT_STA;
  I2C_SEQ_TX_C(dev->seq[0], &dev->buf[0], 1);
  I2C_SEQ_RX_STOP_C(dev->seq[1], &dev->buf[0], 1);
  res = I2C_DEV_sequence(&dev->i2c_stmpe, &dev->seq[0], 2);
  STMPE_FUNC_EXIT;
}

// write
int stmpe811_int_clear(stmpe811_dev *dev, u8_t int_mask) {
  STMPE_FUNC_ENTRY;
  dev->state = STMPE_ST_INT_CLEAR;
  dev->buf[0] = STMPE_REG_INT_STA;
  dev->buf[1] = int_mask;
  I2C_SEQ_TX_STOP_C(dev->seq[0], &dev->buf[0], 2);
  res = I2C_DEV_sequence(&dev->i2c_stmpe, &dev->seq[0], 1);
  STMPE_FUNC_EXIT;
}

// write
int stmpe811_int_gpio_mask(stmpe811_dev *dev, u8_t int_mask) {
  STMPE_FUNC_ENTRY;
  dev->state = STMPE_ST_INT_GPIO_MASK;
  dev->buf[0] = STMPE_REG_GPIO_INT_EN;
  dev->buf[1] = int_mask;
  I2C_SEQ_TX_STOP_C(dev->seq[0], &dev->buf[0], 2);
  res = I2C_DEV_sequence(&dev->i2c_stmpe, &dev->seq[0], 1);
  STMPE_FUNC_EXIT;
}

// read out
int stmpe811_int_gpio_status(stmpe811_dev *dev) {
  STMPE_FUNC_ENTRY;
  dev->state = STMPE_ST_INT_GPIO_STATUS;
  dev->buf[0] = STMPE_REG_GPIO_INT_STA;
  I2C_SEQ_TX_C(dev->seq[0], &dev->buf[0], 1);
  I2C_SEQ_RX_STOP_C(dev->seq[1], &dev->buf[0], 1);
  dev->buf[4] = STMPE_REG_GPIO_INT_STA;
  dev->buf[5] = 0xff;
  I2C_SEQ_TX_STOP_C(dev->seq[2], &dev->buf[4], 2); // clear status bits in gpio reg as erroneously acc to datasheet
  dev->buf[6] = STMPE_REG_GPIO_ED_STA;
  dev->buf[7] = 0xff;
  I2C_SEQ_TX_STOP_C(dev->seq[3], &dev->buf[6], 2); // clear edge status bits in gpio reg
  res = I2C_DEV_sequence(&dev->i2c_stmpe, &dev->seq[0], 4);
  STMPE_FUNC_EXIT;
}

// write
int stmpe811_int_gpio_clear(stmpe811_dev *dev, u8_t int_mask) {
  STMPE_FUNC_ENTRY;
  dev->state = STMPE_ST_INT_GPIO_CLEAR;
  dev->buf[0] = STMPE_REG_GPIO_INT_STA;
  dev->buf[1] = int_mask;
  I2C_SEQ_TX_STOP_C(dev->seq[0], &dev->buf[0], 2);
  res = I2C_DEV_sequence(&dev->i2c_stmpe, &dev->seq[0], 1);
  STMPE_FUNC_EXIT;
}

// write
int stmpe811_int_adc_mask(stmpe811_dev *dev, u8_t int_mask) {
  STMPE_FUNC_ENTRY;
  dev->state = STMPE_ST_INT_ADC_MASK;
  dev->buf[0] = STMPE_REG_ADC_INT_EN;
  dev->buf[1] = int_mask;
  I2C_SEQ_TX_STOP_C(dev->seq[0], &dev->buf[0], 2);
  res = I2C_DEV_sequence(&dev->i2c_stmpe, &dev->seq[0], 1);
  STMPE_FUNC_EXIT;
}

// read out
int stmpe811_int_adc_status(stmpe811_dev *dev) {
  STMPE_FUNC_ENTRY;
  dev->state = STMPE_ST_INT_ADC_STATUS;
  dev->buf[0] = STMPE_REG_ADC_INT_STA;
  I2C_SEQ_TX_C(dev->seq[0], &dev->buf[0], 1);
  I2C_SEQ_RX_STOP_C(dev->seq[1], &dev->buf[0], 1);
  res = I2C_DEV_sequence(&dev->i2c_stmpe, &dev->seq[0], 2);
  STMPE_FUNC_EXIT;
}

// write
int stmpe811_int_adc_clear(stmpe811_dev *dev, u8_t int_mask) {
  STMPE_FUNC_ENTRY;
  dev->state = STMPE_ST_INT_ADC_CLEAR;
  dev->buf[0] = STMPE_REG_ADC_INT_STA;
  dev->buf[1] = int_mask;
  I2C_SEQ_TX_STOP_C(dev->seq[0], &dev->buf[0], 2);
  res = I2C_DEV_sequence(&dev->i2c_stmpe, &dev->seq[0], 1);
  STMPE_FUNC_EXIT;
}

// write fe,re
int stmpe811_gpio_config_detect(stmpe811_dev *dev, u8_t gpio_falling_mask, u8_t gpio_rising_mask) {
  STMPE_FUNC_ENTRY;
  dev->state = STMPE_ST_GPIO_CFG_DETECT;
  dev->buf[0] = STMPE_REG_GPIO_FE;
  dev->buf[1] = gpio_falling_mask;
  dev->buf[2] = STMPE_REG_GPIO_RE;
  dev->buf[3] = gpio_rising_mask;
  I2C_SEQ_TX_STOP_C(dev->seq[0], &dev->buf[0], 2);
  I2C_SEQ_TX_STOP_C(dev->seq[1], &dev->buf[2], 2);
  res = I2C_DEV_sequence(&dev->i2c_stmpe, &dev->seq[0], 2);
  STMPE_FUNC_EXIT;
}

// write
int stmpe811_gpio_config_dir(stmpe811_dev *dev, u8_t gpio_in_out_mask) {
  STMPE_FUNC_ENTRY;
  dev->state = STMPE_ST_GPIO_CFG_DIR;
  dev->buf[0] = STMPE_REG_GPIO_DIR;
  dev->buf[1] = gpio_in_out_mask;
  I2C_SEQ_TX_STOP_C(dev->seq[0], &dev->buf[0], 2);
  res = I2C_DEV_sequence(&dev->i2c_stmpe, &dev->seq[0], 1);
  STMPE_FUNC_EXIT;
}

// read out
int stmpe811_gpio_status(stmpe811_dev *dev) {
  STMPE_FUNC_ENTRY;
  dev->state = STMPE_ST_GPIO_STATUS;
  dev->buf[0] = STMPE_REG_GPIO_MP_STA;
  I2C_SEQ_TX_C(dev->seq[0], &dev->buf[0], 1);
  I2C_SEQ_RX_STOP_C(dev->seq[1], &dev->buf[0], 1);
  res = I2C_DEV_sequence(&dev->i2c_stmpe, &dev->seq[0], 2);
  STMPE_FUNC_EXIT;
}

// read out
int stmpe811_gpio_detect_status(stmpe811_dev *dev) {
  STMPE_FUNC_ENTRY;
  dev->state = STMPE_ST_GPIO_DETECT_STATUS;
  dev->buf[0] = STMPE_REG_GPIO_ED_STA;
  I2C_SEQ_TX_C(dev->seq[0], &dev->buf[0], 1);
  I2C_SEQ_RX_STOP_C(dev->seq[1], &dev->buf[0], 1);
  res = I2C_DEV_sequence(&dev->i2c_stmpe, &dev->seq[0], 2);
  STMPE_FUNC_EXIT;
}

// write
int stmpe811_gpio_set(stmpe811_dev *dev, u8_t gpio_mask) {
  STMPE_FUNC_ENTRY;
  dev->state = STMPE_ST_GPIO_SET;
  dev->buf[0] = STMPE_REG_GPIO_SET_PIN;
  dev->buf[1] = gpio_mask;
  I2C_SEQ_TX_STOP_C(dev->seq[0], &dev->buf[0], 2);
  res = I2C_DEV_sequence(&dev->i2c_stmpe, &dev->seq[0], 1);
  STMPE_FUNC_EXIT;
}

// write
int stmpe811_gpio_clr(stmpe811_dev *dev, u8_t gpio_mask) {
  STMPE_FUNC_ENTRY;
  dev->state = STMPE_ST_GPIO_CLR;
  dev->buf[0] = STMPE_REG_GPIO_CLR_PIN;
  dev->buf[1] = gpio_mask;
  I2C_SEQ_TX_STOP_C(dev->seq[0], &dev->buf[0], 2);
  res = I2C_DEV_sequence(&dev->i2c_stmpe, &dev->seq[0], 1);
  STMPE_FUNC_EXIT;
}

// write adc_ctrl1, adc_ctrl2
int stmpe811_adc_config(stmpe811_dev *dev, stmpe811_adc_cfg_clk_e clk,
    stmpe811_adc_cfg_tim_e time, stmpe811_adc_cfg_res_e resolution, stmpe811_adc_cfg_ref_e reference) {
  STMPE_FUNC_ENTRY;
  u8_t adc_ctrl2 = 0;
  switch (clk) {
  case STMPE_ADC_CLK_1_625MHZ:
    adc_ctrl2 |= STMPE_ADC_CRTRL2_FREQ_1_625;
    break;
  case STMPE_ADC_CLK_3_25MHZ:
    adc_ctrl2 |= STMPE_ADC_CRTRL2_FREQ_3_25;
    break;
  case STMPE_ADC_CLK_6_5MHZ:
    adc_ctrl2 |= STMPE_ADC_CRTRL2_FREQ_6_5;
    break;
  }
  u8_t adc_ctrl1 = 0;
  switch (time) {
  case STMPE_ADC_TIM_36:
    adc_ctrl1 |= STMPE_ADC_CTRL1_SPL_TIME_36;
    break;
  case STMPE_ADC_TIM_44:
    adc_ctrl1 |= STMPE_ADC_CTRL1_SPL_TIME_44;
    break;
  case STMPE_ADC_TIM_56:
    adc_ctrl1 |= STMPE_ADC_CTRL1_SPL_TIME_56;
    break;
  case STMPE_ADC_TIM_64:
    adc_ctrl1 |= STMPE_ADC_CTRL1_SPL_TIME_64;
    break;
  case STMPE_ADC_TIM_80:
    adc_ctrl1 |= STMPE_ADC_CTRL1_SPL_TIME_80;
    break;
  case STMPE_ADC_TIM_96:
    adc_ctrl1 |= STMPE_ADC_CTRL1_SPL_TIME_96;
    break;
  case STMPE_ADC_TIM_124:
    adc_ctrl1 |= STMPE_ADC_CTRL1_SPL_TIME_124;
    break;
  }
  switch (resolution) {
  case STMPE_ADC_RES_10B:
    adc_ctrl1 |= STMPE_ADC_CTRL1_MOD_10B;
    break;
  case STMPE_ADC_RES_12B:
    adc_ctrl1 |= STMPE_ADC_CTRL1_MOD_12B;
    break;
  }
  switch (reference) {
  case STMPE_ADC_REF_EXT:
    adc_ctrl1 |= STMPE_ADC_CTRL1_MOD_REF_EXTNL;
    break;
  case STMPE_ADC_REF_INT:
    adc_ctrl1 |= STMPE_ADC_CTRL1_MOD_REF_INTNL;
    break;
  }
  dev->state = STMPE_ST_ADC_CONF;
  dev->buf[0] = STMPE_REG_ADC_CTRL1;
  dev->buf[1] = adc_ctrl1;
  dev->buf[2] = STMPE_REG_ADC_CTRL2;
  dev->buf[3] = adc_ctrl2;
  I2C_SEQ_TX_STOP_C(dev->seq[0], &dev->buf[0], 2);
  I2C_SEQ_TX_STOP_C(dev->seq[1], &dev->buf[2], 2);
  res = I2C_DEV_sequence(&dev->i2c_stmpe, &dev->seq[0], 2);
  STMPE_FUNC_EXIT;
}

// write
int stmpe811_adc_trigger(stmpe811_dev *dev, u8_t adc_mask) {
  STMPE_FUNC_ENTRY;
  dev->state = STMPE_ST_ADC_TRIG;
  dev->buf[0] = STMPE_REG_ADC_CAPT;
  dev->buf[1] = adc_mask;
  I2C_SEQ_TX_STOP_C(dev->seq[0], &dev->buf[0], 2);
  res = I2C_DEV_sequence(&dev->i2c_stmpe, &dev->seq[0], 1);
  STMPE_FUNC_EXIT;
}

// read out
int stmpe811_adc_status(stmpe811_dev *dev) {
  STMPE_FUNC_ENTRY;
  dev->state = STMPE_ST_ADC_STAT;
  dev->buf[0] = STMPE_REG_ADC_INT_STA;
  I2C_SEQ_TX_C(dev->seq[0], &dev->buf[0], 1);
  I2C_SEQ_RX_STOP_C(dev->seq[1], &dev->buf[0], 1);
  dev->buf[4] = STMPE_REG_ADC_INT_STA;
  dev->buf[5] = 0xff;
  I2C_SEQ_TX_STOP_C(dev->seq[2], &dev->buf[4], 2); // clear status bits in adc reg as erroneously acc to datasheet
  res = I2C_DEV_sequence(&dev->i2c_stmpe, &dev->seq[0], 3);
  STMPE_FUNC_EXIT;
}

// read out
int stmpe811_adc_read(stmpe811_dev *dev, stmpe811_adc_e adc_ch) {
  STMPE_FUNC_ENTRY;
  dev->state = STMPE_ST_ADC_READ;

  switch (adc_ch) {
  case STMPE_ADC_CHAN0:
    dev->buf[0] = STMPE_REG_ADC_DATA_CH0;
    break;
  case STMPE_ADC_CHAN1:
    dev->buf[0] = STMPE_REG_ADC_DATA_CH1;
    break;
  case STMPE_ADC_CHAN2:
    dev->buf[0] = STMPE_REG_ADC_DATA_CH2;
    break;
  case STMPE_ADC_CHAN3:
    dev->buf[0] = STMPE_REG_ADC_DATA_CH3;
    break;
  case STMPE_ADC_CHAN4:
    dev->buf[0] = STMPE_REG_ADC_DATA_CH4;
    break;
  case STMPE_ADC_CHAN5:
    dev->buf[0] = STMPE_REG_ADC_DATA_CH5;
    break;
  case STMPE_ADC_CHAN6:
    dev->buf[0] = STMPE_REG_ADC_DATA_CH6;
    break;
  case STMPE_ADC_CHAN7:
    dev->buf[0] = STMPE_REG_ADC_DATA_CH7;
    break;
  }

  I2C_SEQ_TX_C(dev->seq[0], &dev->buf[0], 1);
  I2C_SEQ_RX_STOP_C(dev->seq[1], &dev->buf[0], 2);
  res = I2C_DEV_sequence(&dev->i2c_stmpe, &dev->seq[0], 2);
  STMPE_FUNC_EXIT;
}

// read out, write modified config bits
int stmpe811_temp_config(stmpe811_dev *dev, bool enable, stmpe811_temp_cfg_mode_e mode) {
  STMPE_FUNC_ENTRY;
  dev->state = STMPE_ST_TEMP_CFG;
  u8_t cfg = 0;
  cfg |= enable ? STMPE_TEMP_CTRL_ENABLE_ON : STMPE_TEMP_CTRL_ENABLE_OFF;
  switch (mode) {
  case STMPE_TEMP_MODE_ONCE:
    cfg |= STMPE_TEMP_CTRL_ACQ_MOD_ONCE;
    break;
  case STMPE_TEMP_MODE_EVERY_10_MS:
    cfg |= STMPE_TEMP_CTRL_ACQ_MOD_CONT;
    break;
  }
  dev->arg = cfg;
  dev->buf[0] = STMPE_REG_TEMP_CTRL;
  I2C_SEQ_TX_C(dev->seq[0], &dev->buf[0], 1);
  I2C_SEQ_RX_STOP_C(dev->seq[1], &dev->buf[1], 1);
  res = I2C_DEV_sequence(&dev->i2c_stmpe, &dev->seq[0], 2);
  STMPE_FUNC_EXIT;
}

// read out, write modified config bits, write thres
int stmpe811_temp_config_threshold(stmpe811_dev *dev, bool enable, stmpe811_temp_cfg_thres_e mode,
    u16_t threshold) {
  STMPE_FUNC_ENTRY;
  dev->state = STMPE_ST_TEMP_CFG_TH;
  u8_t cfg = 0;
  cfg |= enable ? STMPE_TEMP_CTRL_THRES_EN_ON : STMPE_TEMP_CTRL_THRES_EN_OFF;
  switch (mode) {
  case STMPE_TEMP_THRES_OVER:
    cfg |= STMPE_TEMP_CTRL_THRES_RANGE_OVER;
    break;
  case STMPE_TEMP_THRES_UNDER:
    cfg |= STMPE_TEMP_CTRL_THRES_RANGE_UNDER;
    break;
  }
  dev->arg = threshold;
  dev->buf[2] = cfg;
  dev->buf[0] = STMPE_REG_TEMP_CTRL;
  I2C_SEQ_TX_C(dev->seq[0], &dev->buf[0], 1);
  I2C_SEQ_RX_STOP_C(dev->seq[1], &dev->buf[1], 1);
  res = I2C_DEV_sequence(&dev->i2c_stmpe, &dev->seq[0], 2);
  STMPE_FUNC_EXIT;
}

// read out, write modified config bits
int stmpe811_temp_acquire(stmpe811_dev *dev, bool acquire) {
  STMPE_FUNC_ENTRY;
  dev->state = STMPE_ST_TEMP_ACQ;
  u8_t cfg = 0;
  cfg |= acquire ? STMPE_TEMP_CTRL_ACQ_ON : STMPE_TEMP_CTRL_ACQ_OFF;
  dev->arg = cfg;
  dev->buf[0] = STMPE_REG_TEMP_CTRL;
  I2C_SEQ_TX_C(dev->seq[0], &dev->buf[0], 1);
  I2C_SEQ_RX_STOP_C(dev->seq[1], &dev->buf[1], 1);
  res = I2C_DEV_sequence(&dev->i2c_stmpe, &dev->seq[0], 2);
  STMPE_FUNC_EXIT;
}

// read out
int stmpe811_temp_read(stmpe811_dev *dev) {
  STMPE_FUNC_ENTRY;
  dev->state = STMPE_ST_TEMP_READ;
  dev->buf[0] = STMPE_REG_TEMP_DATA_H;
  I2C_SEQ_TX_C(dev->seq[0], &dev->buf[0], 1);
  I2C_SEQ_RX_STOP_C(dev->seq[1], &dev->buf[0], 2);
  res = I2C_DEV_sequence(&dev->i2c_stmpe, &dev->seq[0], 2);
  STMPE_FUNC_EXIT;
}

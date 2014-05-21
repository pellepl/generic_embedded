/*
 * stmpe811_driver.h
 *
 *  Created on: Feb 28, 2014
 *      Author: petera
 */

#ifndef STMPE811_DRIVER_H_
#define STMPE811_DRIVER_H_

#include "system.h"
#include "i2c_dev.h"

#define I2C_ERR_STMPE811_ID               -1300
#define I2C_ERR_STMPE811_UNKNOWN_STATE    -1301


// hardware mappings

#define STMPE_BLOCK_TEMP                  (1<<3)
#define STMPE_BLOCK_GPIO                  (1<<2)
#define STMPE_BLOCK_TSC                   (1<<1)
#define STMPE_BLOCK_ADC                   (1<<0)

#define STMPE_INT_GPIO                    (1<<7)
#define STMPE_INT_ADC                     (1<<6)
#define STMPE_INT_TEMP_SENS               (1<<5)
#define STMPE_INT_FIFO_EMPTY              (1<<4)
#define STMPE_INT_FIFO_FULL               (1<<3)
#define STMPE_INT_FIFO_OFLOW              (1<<2)
#define STMPE_INT_FIFO_TH                 (1<<1)
#define STMPE_INT_TOUCH_DET               (1<<0)

#define STMPE_GPIO7                       (1<<7)
#define STMPE_GPIO6                       (1<<6)
#define STMPE_GPIO5                       (1<<5)
#define STMPE_GPIO4                       (1<<4)
#define STMPE_GPIO3                       (1<<3)
#define STMPE_GPIO2                       (1<<2)
#define STMPE_GPIO1                       (1<<1)
#define STMPE_GPIO0                       (1<<0)

#define STMPE_ADC7                        (1<<7)
#define STMPE_ADC6                        (1<<6)
#define STMPE_ADC5                        (1<<5)
#define STMPE_ADC4                        (1<<4)
#define STMPE_ADC3                        (1<<3)
#define STMPE_ADC2                        (1<<2)
#define STMPE_ADC1                        (1<<1)
#define STMPE_ADC0                        (1<<0)

  // weird mapping - see data sheet p 29

#define STMPE_GPIO_ADC7                   STMPE_ADC3
#define STMPE_GPIO_ADC6                   STMPE_ADC1
#define STMPE_GPIO_ADC5                   STMPE_ADC2
#define STMPE_GPIO_ADC4                   STMPE_ADC0
#define STMPE_GPIO_ADC3                   STMPE_ADC7
#define STMPE_GPIO_ADC2                   STMPE_ADC6
#define STMPE_GPIO_ADC1                   STMPE_ADC5
#define STMPE_GPIO_ADC0                   STMPE_ADC4

// ADC
//  HW    DATA_REG
//  CH7 = CH3
//  CH6 = CH1
//  CH5 = CH2
//  CH4 = CH0
//  CH3 = CH7
//  CH2 = CH6
//  CH1 = CH5
//  CH0 = CH4
//

// software driver

typedef enum {
  STMPE_ST_ERROR = -1,
  STMPE_ST_IDLE = 0,
  STMPE_ST_IDENTIFY,
  STMPE_ST_SOFT_RESET,
  STMPE_ST_SOFT_RESET1,
  STMPE_ST_SOFT_RESET2,
  STMPE_ST_HIBERNATE,
  STMPE_ST_HIBERNATE1,
  STMPE_ST_CTRL_POW_BLOCK,
  STMPE_ST_CTRL_POW_BLOCK1,
  STMPE_ST_CTRL_CONFIG_PORT_AF,
  STMPE_ST_INT_CONFIG,
  STMPE_ST_INT_CONFIG1,
  STMPE_ST_INT_ENABLE,
  STMPE_ST_INT_ENABLE1,
  STMPE_ST_INT_MASK,
  STMPE_ST_INT_STATUS,
  STMPE_ST_INT_CLEAR,
  STMPE_ST_INT_GPIO_MASK,
  STMPE_ST_INT_GPIO_STATUS,
  STMPE_ST_INT_GPIO_CLEAR,
  STMPE_ST_INT_ADC_MASK,
  STMPE_ST_INT_ADC_STATUS,
  STMPE_ST_INT_ADC_CLEAR,
  STMPE_ST_GPIO_CFG_DETECT,
  STMPE_ST_GPIO_CFG_DIR,
  STMPE_ST_GPIO_STATUS,
  STMPE_ST_GPIO_DETECT_STATUS,
  STMPE_ST_GPIO_SET,
  STMPE_ST_GPIO_CLR,
  STMPE_ST_ADC_CONF,
  STMPE_ST_ADC_TRIG,
  STMPE_ST_ADC_STAT,
  STMPE_ST_ADC_READ,
  STMPE_ST_TEMP_CFG,
  STMPE_ST_TEMP_CFG1,
  STMPE_ST_TEMP_CFG_TH,
  STMPE_ST_TEMP_CFG_TH1,
  STMPE_ST_TEMP_ACQ,
  STMPE_ST_TEMP_ACQ1,
  STMPE_ST_TEMP_READ,
} stmpe811_state_e;

typedef struct stmpe811_dev_s {
  i2c_dev i2c_stmpe;
  stmpe811_state_e state;
  void (*callback)(struct stmpe811_dev_s *dev, int res,
      stmpe811_state_e state, u16_t arg);
  i2c_dev_sequence seq[4];
  u8_t buf[8];
  u16_t arg;
} stmpe811_dev;

typedef enum {
  STMPE_INT_POLARITY_ACTIVE_LOW_FALLING = 0,
  STMPE_INT_POLARITY_ACTIVE_HIGH_RISING
} stmpe_int_cfg_pol_e;

typedef enum {
  STMPE_INT_TYPE_LEVEL = 0,
  STMPE_INT_TYPE_EDGE
} stmpe_int_cfg_typ_e;

typedef enum {
  STMPE_ADC_CLK_1_625MHZ = 0,
  STMPE_ADC_CLK_3_25MHZ,
  STMPE_ADC_CLK_6_5MHZ
} stmpe811_adc_cfg_clk_e;

typedef enum {
  STMPE_ADC_TIM_36 = 0,
  STMPE_ADC_TIM_44,
  STMPE_ADC_TIM_56,
  STMPE_ADC_TIM_64,
  STMPE_ADC_TIM_80,
  STMPE_ADC_TIM_96,
  STMPE_ADC_TIM_124,
} stmpe811_adc_cfg_tim_e;

typedef enum {
  STMPE_ADC_RES_12B = 0,
  STMPE_ADC_RES_10B
} stmpe811_adc_cfg_res_e;

typedef enum {
  STMPE_ADC_REF_INT = 0,
  STMPE_ADC_REF_EXT
} stmpe811_adc_cfg_ref_e;

typedef enum {
  STMPE_ADC_CHAN0 = 0,
  STMPE_ADC_CHAN1,
  STMPE_ADC_CHAN2,
  STMPE_ADC_CHAN3,
  STMPE_ADC_CHAN4,
  STMPE_ADC_CHAN5,
  STMPE_ADC_CHAN6,
  STMPE_ADC_CHAN7,
} stmpe811_adc_e;

#define STMPE_GPIO_ADC_CHAN0 STMPE_ADC_CHAN4
#define STMPE_GPIO_ADC_CHAN1 STMPE_ADC_CHAN5
#define STMPE_GPIO_ADC_CHAN2 STMPE_ADC_CHAN6
#define STMPE_GPIO_ADC_CHAN3 STMPE_ADC_CHAN7
#define STMPE_GPIO_ADC_CHAN4 STMPE_ADC_CHAN0
#define STMPE_GPIO_ADC_CHAN5 STMPE_ADC_CHAN2
#define STMPE_GPIO_ADC_CHAN6 STMPE_ADC_CHAN1
#define STMPE_GPIO_ADC_CHAN7 STMPE_ADC_CHAN3

typedef enum {
  STMPE_TEMP_MODE_ONCE = 0,
  STMPE_TEMP_MODE_EVERY_10_MS
} stmpe811_temp_cfg_mode_e;

typedef enum {
  STMPE_TEMP_THRES_UNDER = 0,
  STMPE_TEMP_THRES_OVER
} stmpe811_temp_cfg_thres_e;

void stmpe811_open(stmpe811_dev *dev, i2c_bus *bus,  void (*callback)(struct stmpe811_dev_s *dev, int res,
    stmpe811_state_e state, u16_t arg));
void stmpe811_close(stmpe811_dev *dev);

int stmpe811_identify(stmpe811_dev *dev);

int stmpe811_ctrl_reset_soft(stmpe811_dev *dev);
int stmpe811_ctrl_hibernate(stmpe811_dev *dev, bool hibernate);
int stmpe811_ctrl_power_block(stmpe811_dev *dev, u8_t block_enable_mask, u8_t block_disable_mask);
int stmpe811_ctrl_config_port_af(stmpe811_dev *dev, u8_t gpio_analog_digital_mask);

int stmpe811_int_config(stmpe811_dev *dev, stmpe_int_cfg_pol_e polarity, stmpe_int_cfg_typ_e type);
int stmpe811_int_enable(stmpe811_dev *dev);
int stmpe811_int_disable(stmpe811_dev *dev);
int stmpe811_int_mask(stmpe811_dev *dev, u8_t int_mask);
int stmpe811_int_status(stmpe811_dev *dev);
int stmpe811_int_clear(stmpe811_dev *dev, u8_t int_mask);
int stmpe811_int_gpio_mask(stmpe811_dev *dev, u8_t gpio_mask);
int stmpe811_int_gpio_status(stmpe811_dev *dev);
int stmpe811_int_gpio_clear(stmpe811_dev *dev, u8_t gpio_mask);
int stmpe811_int_adc_mask(stmpe811_dev *dev, u8_t adc_mask);
int stmpe811_int_adc_status(stmpe811_dev *dev);
int stmpe811_int_adc_clear(stmpe811_dev *dev, u8_t adc_mask);

int stmpe811_gpio_config_detect(stmpe811_dev *dev, u8_t gpio_falling_mask, u8_t gpio_rising_mask);
int stmpe811_gpio_config_dir(stmpe811_dev *dev, u8_t gpio_in_out_mask);
int stmpe811_gpio_status(stmpe811_dev *dev);
int stmpe811_gpio_detect_status(stmpe811_dev *dev);
int stmpe811_gpio_set(stmpe811_dev *dev, u8_t gpio_mask);
int stmpe811_gpio_clr(stmpe811_dev *dev, u8_t gpio_mask);

int stmpe811_adc_config(stmpe811_dev *dev, stmpe811_adc_cfg_clk_e clk,
    stmpe811_adc_cfg_tim_e time, stmpe811_adc_cfg_res_e resolution, stmpe811_adc_cfg_ref_e reference);
int stmpe811_adc_trigger(stmpe811_dev *dev, u8_t adc_mask);
int stmpe811_adc_status(stmpe811_dev *dev);
int stmpe811_adc_read(stmpe811_dev *dev, stmpe811_adc_e adc_ch);

int stmpe811_temp_config(stmpe811_dev *dev, bool enable, stmpe811_temp_cfg_mode_e mode);
int stmpe811_temp_config_threshold(stmpe811_dev *dev, bool enable, stmpe811_temp_cfg_thres_e mode,
    u16_t threshold);
int stmpe811_temp_acquire(stmpe811_dev *dev, bool acquire);
int stmpe811_temp_read(stmpe811_dev *dev);

#endif /* STMPE811_DRIVER_H_ */

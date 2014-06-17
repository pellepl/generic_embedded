/*
 * stmpe811_handler.h
 *
 *  Created on: Apr 7, 2014
 *      Author: petera
 */

#ifndef STMPE811_HANDLER_H_
#define STMPE811_HANDLER_H_

#include "stmpe811_driver.h"

typedef enum {
  STMPE811_HDL_STATE_IDLE = 0,

  STMPE811_HDL_STATE_SETUP_ID,
  STMPE811_HDL_STATE_SETUP_RESET,
  STMPE811_HDL_STATE_SETUP_POWER_BLOCK,
  STMPE811_HDL_STATE_SETUP_ANALOG_DIGITAL_MASK,
  STMPE811_HDL_STATE_SETUP_DEFAULT_OUTPUT_DISA,
  STMPE811_HDL_STATE_SETUP_DEFAULT_OUTPUT_ENA,
  STMPE811_HDL_STATE_SETUP_GPIO_DIRECTION,
  STMPE811_HDL_STATE_SETUP_INT_CFG,
  STMPE811_HDL_STATE_SETUP_INT_MASK,
  STMPE811_HDL_STATE_SETUP_INT_GPIO_MASK,
  STMPE811_HDL_STATE_SETUP_INT_ADC_MASK,
  STMPE811_HDL_STATE_SETUP_INT_ENABLE,
  STMPE811_HDL_STATE_SETUP_ADC_CFG,
  STMPE811_HDL_STATE_SETUP_TEMP_CFG,
  STMPE811_HDL_STATE_SETUP_TEMP_THRES_CFG,
  STMPE811_HDL_STATE_SETUP_FINISHED,

  STMPE811_HDL_STATE_INT_ENTRY,
  STMPE811_HDL_STATE_INT_HANDLE,
  STMPE811_HDL_STATE_INT_GPIO_CLR,
  STMPE811_HDL_STATE_INT_GPIO_READ,
  STMPE811_HDL_STATE_INT_GPIO_READ_STATUS,
  STMPE811_HDL_STATE_INT_ADC_CLR,
  STMPE811_HDL_STATE_INT_ADC_STATUS,
  STMPE811_HDL_STATE_INT_ADC_CHECK,
  STMPE811_HDL_STATE_INT_ADC_READ,
  STMPE811_HDL_STATE_INT_TEMP_CLR,
  STMPE811_HDL_STATE_INT_TEMP_READ,

  STMPE811_HDL_STATE_GPIO_DEFINE_CLR,
  STMPE811_HDL_STATE_GPIO_DEFINE_SET,

  STMPE811_HDL_STATE_ADC_READ,

  STMPE811_HDL_STATE_TEMP_READ,
  STMPE811_HDL_STATE_TEMP_RESULT,

  STMPE811_HDL_STATE_INT_STA_READ,

} stmpe811_handler_state;

typedef void (*stmpe811_handler_gpio_cb_fn)(u8_t gpio_mask, u8_t gpio_value);
typedef void (*stmpe811_handler_adc_cb_fn)(u8_t adc, u16_t value);
typedef void (*stmpe811_handler_temp_cb_fn)(u16_t value);
typedef void (*stmpe811_handler_err_cb_fn)(stmpe811_handler_state state, int err);

typedef struct stmpe811_handler_s {
  stmpe811_dev dev;
  stmpe811_handler_state state;
  stmpe811_handler_gpio_cb_fn gpio_cb;
  stmpe811_handler_adc_cb_fn adc_cb;
  stmpe811_handler_temp_cb_fn temp_cb;
  stmpe811_handler_err_cb_fn err_cb;
  struct {
    u8_t power_block;               // STMPE_BLOCK_* combo
    u8_t gpio_analog_digital_mask;  // bit lo = ana, bit hi = gpio, STMPE_GPIO_* combo
    u8_t gpio_direction;            // bit lo = inp, bit hi = out, STMPE_GPIO_* combo
    u8_t gpio_default_output;       // STMPE_GPIO_* combo

    bool interrupt;                 // TRUE = enable interrupts, FALSE = disable interrupts
    stmpe_int_cfg_pol_e interrupt_polarity;
    stmpe_int_cfg_typ_e interrupt_type;
    u8_t interrupt_mask;            // STMPE_INT_* combo
    u8_t gpio_interrupt_mask;       // STMPE_GPIO_* combo
    u8_t adc_interrupt_mask;        // STMPE_ADC_* combo

    u8_t gpio_detect_rising;        // STMPE_GPIO_* combo
    u8_t gpio_detect_falling;       // STMPE_GPIO_* combo

    stmpe811_adc_cfg_clk_e adc_clk;
    stmpe811_adc_cfg_tim_e adc_time;
    stmpe811_adc_cfg_res_e adc_resolution;
    stmpe811_adc_cfg_ref_e adc_reference;

    bool temp_enable;
    stmpe811_temp_cfg_mode_e temp_mode;
    bool temp_threshold_enable;
    stmpe811_temp_cfg_thres_e temp_thres_mode;
    u16_t temp_threshold;
  } cfg;
  u8_t int_status;
  u8_t gpio_state;
  u8_t adc_state;
  u8_t adc_chan;
  void *user_data;
} stmpe811_handler;

void stmpe811_handler_open(stmpe811_handler *hdl, i2c_bus *bus,
    stmpe811_handler_gpio_cb_fn gpio_cb,
    stmpe811_handler_adc_cb_fn adc_cb,
    stmpe811_handler_temp_cb_fn temp_cb,
    stmpe811_handler_err_cb_fn err_cb);

void stmpe811_handler_close(stmpe811_handler *hdl);

int stmpe811_handler_setup(
    stmpe811_handler *hdl,
    u8_t power_block,               // STMPE_BLOCK_* combo
    u8_t gpio_analog_digital_mask,  // bit lo = ana, bit hi = gpio, STMPE_GPIO_* combo
    u8_t gpio_direction,            // bit lo = inp, bit hi = out, STMPE_GPIO_* combo
    u8_t gpio_default_output,       // STMPE_GPIO_* combo

    bool interrupt,                 // TRUE = enable interrupts, FALSE = disable interrupts
    stmpe_int_cfg_pol_e interrupt_polarity, stmpe_int_cfg_typ_e interrupt_type,
    u8_t interrupt_mask,            // STMPE_INT_* combo
    u8_t gpio_interrupt_mask,       // STMPE_GPIO_* combo
    u8_t adc_interrupt_mask,        // STMPE_ADC_* combo

    u8_t gpio_detect_rising,        // STMPE_GPIO_* combo
    u8_t gpio_detect_falling,       // STMPE_GPIO_* combo

    stmpe811_adc_cfg_clk_e adc_clk,
    stmpe811_adc_cfg_tim_e adc_time,
    stmpe811_adc_cfg_res_e adc_resolution,
    stmpe811_adc_cfg_ref_e adc_reference,

    bool temp_enable,
    stmpe811_temp_cfg_mode_e temp_mode,
    bool temp_threshold_enable,
    stmpe811_temp_cfg_thres_e temp_thres_mode,
    u16_t temp_threshold);

int stmpe811_handler_gpio_define(stmpe811_handler *hdl, u8_t set, u8_t reset);

int stmpe811_handler_adc_read(stmpe811_handler *hdl, u8_t adc);

int stmpe811_handler_temp_read(stmpe811_handler *hdl, bool enable);

int stmpe811_handler_int_sta_read(stmpe811_handler *hdl);

u8_t stmpe811_handler_int_sta_get(stmpe811_handler *hdl);

/* this function is to be called when an interrupt occurs on the stmpe811 */
int stmpe811_handler_interrupt_cb(stmpe811_handler *hdl);

#endif /* STMPE811_HANDLER_H_ */

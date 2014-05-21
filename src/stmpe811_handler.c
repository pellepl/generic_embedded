/*
 * stmpe811_handler.c
 *
 *  Created on: Apr 7, 2014
 *      Author: petera
 */

#include "stmpe811_handler.h"

#define STMPE_H_DBG(...)

static int _stmpe811_handler_int_handle(stmpe811_handler *hdl, u16_t arg) {
  int res = I2C_OK;
  stmpe811_handler_state ostate = hdl->state;
  switch (ostate) {

  // check interrupt status
  case STMPE811_HDL_STATE_INT_HANDLE:
    if (hdl->int_status & STMPE_INT_GPIO) {
      // gpio interrupt
      STMPE_H_DBG("stmpe irq gpio\n");
      hdl->state = STMPE811_HDL_STATE_INT_GPIO_CLR;
      res = stmpe811_int_clear(&hdl->dev, STMPE_INT_GPIO);
    } else if (hdl->int_status & STMPE_INT_ADC) {
      // adc interrupt
      STMPE_H_DBG("stmpe irq adc\n");
      hdl->state = STMPE811_HDL_STATE_INT_ADC_CLR;
      res = stmpe811_int_clear(&hdl->dev, STMPE_INT_ADC);
    } else if (hdl->int_status & STMPE_INT_TEMP_SENS) {
      // temp interrupt
      STMPE_H_DBG("stmpe irq temp\n");
      hdl->state = STMPE811_HDL_STATE_INT_TEMP_CLR;
      res = stmpe811_int_clear(&hdl->dev, STMPE_INT_TEMP_SENS);
    } else {
      // no more interrupts, report ok
      hdl->state = STMPE811_HDL_STATE_IDLE;
      STMPE_H_DBG("stmpe irq end\n");
      if (hdl->err_cb) hdl->err_cb(ostate, res);
      res = I2C_OK;
    }
    break;

  // gpio operations
  case STMPE811_HDL_STATE_INT_GPIO_CLR:
    hdl->state = STMPE811_HDL_STATE_INT_GPIO_READ;
    res = stmpe811_gpio_status(&hdl->dev);
    break;
  case STMPE811_HDL_STATE_INT_GPIO_READ:
    hdl->gpio_state = arg;
    hdl->state = STMPE811_HDL_STATE_INT_GPIO_READ_STATUS;
    res = stmpe811_int_gpio_status(&hdl->dev);
    break;
  case STMPE811_HDL_STATE_INT_GPIO_READ_STATUS:
    if (hdl->gpio_cb && arg > 0) hdl->gpio_cb(arg, hdl->gpio_state);
    // reread interrupt status
    hdl->state = STMPE811_HDL_STATE_INT_ENTRY;
    res = stmpe811_int_status(&hdl->dev);
    break;

  // adc operations
  case STMPE811_HDL_STATE_INT_ADC_CLR:
    hdl->state = STMPE811_HDL_STATE_INT_ADC_STATUS;
    res = stmpe811_adc_status(&hdl->dev);
    break;
  case STMPE811_HDL_STATE_INT_ADC_STATUS:
    hdl->adc_state = arg;
    hdl->adc_chan = 0;
    STMPE_H_DBG("adc stat:%08b\n", arg);
    // no break
  case STMPE811_HDL_STATE_INT_ADC_CHECK:
  case STMPE811_HDL_STATE_INT_ADC_READ:
    if (hdl->adc_chan && hdl->adc_cb) {
      hdl->adc_cb(hdl->adc_chan, arg);
    }
    if (hdl->adc_state & STMPE_ADC0) {
      hdl->adc_state &= ~STMPE_ADC0;
      hdl->adc_chan = STMPE_ADC0;
      hdl->state = STMPE811_HDL_STATE_INT_ADC_READ;
      res = stmpe811_adc_read(&hdl->dev, STMPE_ADC_CHAN0);
    } else if (hdl->adc_state & STMPE_ADC1) {
      hdl->adc_state &= ~STMPE_ADC1;
      hdl->adc_chan = STMPE_ADC1;
      hdl->state = STMPE811_HDL_STATE_INT_ADC_READ;
      res = stmpe811_adc_read(&hdl->dev, STMPE_ADC_CHAN1);
    } else if (hdl->adc_state & STMPE_ADC2) {
      hdl->adc_state &= ~STMPE_ADC2;
      hdl->adc_chan = STMPE_ADC2;
      hdl->state = STMPE811_HDL_STATE_INT_ADC_READ;
      res = stmpe811_adc_read(&hdl->dev, STMPE_ADC_CHAN2);
    } else if (hdl->adc_state & STMPE_ADC3) {
      hdl->adc_state &= ~STMPE_ADC3;
      hdl->adc_chan = STMPE_ADC3;
      hdl->state = STMPE811_HDL_STATE_INT_ADC_READ;
      res = stmpe811_adc_read(&hdl->dev, STMPE_ADC_CHAN3);
    } else if (hdl->adc_state & STMPE_ADC4) {
      hdl->adc_state &= ~STMPE_ADC4;
      hdl->adc_chan = STMPE_ADC4;
      hdl->state = STMPE811_HDL_STATE_INT_ADC_READ;
      res = stmpe811_adc_read(&hdl->dev, STMPE_ADC_CHAN4);
    } else if (hdl->adc_state & STMPE_ADC5) {
      hdl->adc_state &= ~STMPE_ADC5;
      hdl->adc_chan = STMPE_ADC5;
      hdl->state = STMPE811_HDL_STATE_INT_ADC_READ;
      res = stmpe811_adc_read(&hdl->dev, STMPE_ADC_CHAN5);
    } else if (hdl->adc_state & STMPE_ADC6) {
      hdl->adc_state &= ~STMPE_ADC6;
      hdl->adc_chan = STMPE_ADC6;
      hdl->state = STMPE811_HDL_STATE_INT_ADC_READ;
      res = stmpe811_adc_read(&hdl->dev, STMPE_ADC_CHAN6);
    } else if (hdl->adc_state & STMPE_ADC7) {
      hdl->adc_state &= ~STMPE_ADC7;
      hdl->adc_chan = STMPE_ADC7;
      hdl->state = STMPE811_HDL_STATE_INT_ADC_READ;
      res = stmpe811_adc_read(&hdl->dev, STMPE_ADC_CHAN7);
    } else {
      // no more adc to read
      // reread interrupt status
      hdl->state = STMPE811_HDL_STATE_INT_ENTRY;
      res = stmpe811_int_status(&hdl->dev);
    }
    break;

  // temp operations
  case STMPE811_HDL_STATE_INT_TEMP_CLR:
    hdl->state = STMPE811_HDL_STATE_INT_TEMP_READ;
    res = stmpe811_temp_read(&hdl->dev);
    break;
  case STMPE811_HDL_STATE_INT_TEMP_READ:
    if (hdl->temp_cb) hdl->temp_cb(arg);
    // reread interrupt status
    hdl->state = STMPE811_HDL_STATE_INT_ENTRY;
    res = stmpe811_int_status(&hdl->dev);
    break;
  default:
    res = I2C_ERR_STMPE811_UNKNOWN_STATE;
    break;
  }

  return res;
}

static void _stmpe811_cb(struct stmpe811_dev_s *dev, int res,
    stmpe811_state_e raw_state, u16_t arg) {
  stmpe811_handler *hdl = (stmpe811_handler *)dev->i2c_stmpe.user_data;
  stmpe811_handler_state state = hdl->state;
  if (res != I2C_OK) {
    hdl->state = STMPE811_HDL_STATE_IDLE;
    if (hdl->err_cb) hdl->err_cb(state, res);
    return;
  }

  switch (state) {

  // temp read

  case STMPE811_HDL_STATE_TEMP_READ:
    hdl->state = STMPE811_HDL_STATE_TEMP_RESULT;
    res = stmpe811_temp_read(&hdl->dev);
    break;
  case STMPE811_HDL_STATE_TEMP_RESULT:
    hdl->state = STMPE811_HDL_STATE_IDLE;
    if (hdl->temp_cb) hdl->temp_cb(arg);
    if (hdl->err_cb) hdl->err_cb(hdl->state, res);
    res = I2C_OK;
    break;

  // adc read

  case STMPE811_HDL_STATE_ADC_READ:
    hdl->state = STMPE811_HDL_STATE_IDLE;
    if (hdl->err_cb) hdl->err_cb(state, res);
    res = I2C_OK;
    break;

  // gpio define

  case STMPE811_HDL_STATE_GPIO_DEFINE_CLR:
    hdl->state = STMPE811_HDL_STATE_GPIO_DEFINE_SET;
    res = stmpe811_gpio_set(&hdl->dev, hdl->gpio_state);
    break;
  case STMPE811_HDL_STATE_GPIO_DEFINE_SET:
    hdl->state = STMPE811_HDL_STATE_IDLE;
    if (hdl->err_cb) hdl->err_cb(state, res);
    res = I2C_OK;
    break;

  // interrupts

  case STMPE811_HDL_STATE_INT_ENTRY:
    hdl->int_status = arg;
    hdl->state = STMPE811_HDL_STATE_INT_HANDLE;
    // no break
  case STMPE811_HDL_STATE_INT_HANDLE:
  case STMPE811_HDL_STATE_INT_GPIO_CLR:
  case STMPE811_HDL_STATE_INT_GPIO_READ:
  case STMPE811_HDL_STATE_INT_GPIO_READ_STATUS:
  case STMPE811_HDL_STATE_INT_ADC_CLR:
  case STMPE811_HDL_STATE_INT_ADC_STATUS:
  case STMPE811_HDL_STATE_INT_ADC_CHECK:
  case STMPE811_HDL_STATE_INT_ADC_READ:
  case STMPE811_HDL_STATE_INT_TEMP_CLR:
  case STMPE811_HDL_STATE_INT_TEMP_READ:
    res = _stmpe811_handler_int_handle(hdl, arg);
    break;

  // setup

  case STMPE811_HDL_STATE_SETUP_ID:
    if (arg != 0x0811) {
      res = I2C_ERR_STMPE811_ID;
    } else {
      hdl->state = STMPE811_HDL_STATE_SETUP_RESET;
      res = stmpe811_ctrl_reset_soft(&hdl->dev);
    }
    break;

  case STMPE811_HDL_STATE_SETUP_RESET:
    hdl->state = STMPE811_HDL_STATE_SETUP_POWER_BLOCK;
    res = stmpe811_ctrl_power_block(&hdl->dev, hdl->cfg.power_block,
        (~hdl->cfg.power_block) & (STMPE_BLOCK_TEMP | STMPE_BLOCK_TSC | STMPE_BLOCK_GPIO | STMPE_BLOCK_ADC));

    break;

  case STMPE811_HDL_STATE_SETUP_POWER_BLOCK:
    hdl->state = STMPE811_HDL_STATE_SETUP_ANALOG_DIGITAL_MASK;
    res = stmpe811_ctrl_config_port_af(&hdl->dev, hdl->cfg.gpio_analog_digital_mask);
    break;

  case STMPE811_HDL_STATE_SETUP_ANALOG_DIGITAL_MASK:
    hdl->state = STMPE811_HDL_STATE_SETUP_DEFAULT_OUTPUT_DISA;
    res = stmpe811_gpio_clr(&hdl->dev, ~hdl->cfg.gpio_default_output);
    break;

  case STMPE811_HDL_STATE_SETUP_DEFAULT_OUTPUT_DISA:
    hdl->state = STMPE811_HDL_STATE_SETUP_DEFAULT_OUTPUT_ENA;
    res = stmpe811_gpio_set(&hdl->dev, hdl->cfg.gpio_default_output);
    break;

  case STMPE811_HDL_STATE_SETUP_DEFAULT_OUTPUT_ENA:
    hdl->state = STMPE811_HDL_STATE_SETUP_GPIO_DIRECTION;
    res = stmpe811_gpio_config_dir(&hdl->dev, hdl->cfg.gpio_direction);
    break;

  case STMPE811_HDL_STATE_SETUP_GPIO_DIRECTION:
    hdl->state = STMPE811_HDL_STATE_SETUP_INT_CFG;
    res = stmpe811_int_config(&hdl->dev,
        hdl->cfg.interrupt_polarity,
        hdl->cfg.interrupt_type);
    break;

  case STMPE811_HDL_STATE_SETUP_INT_CFG:
    hdl->state = STMPE811_HDL_STATE_SETUP_INT_MASK;
    res = stmpe811_int_mask(&hdl->dev, hdl->cfg.interrupt_mask);
    break;

  case STMPE811_HDL_STATE_SETUP_INT_MASK:
    hdl->state = STMPE811_HDL_STATE_SETUP_INT_GPIO_MASK;
    res = stmpe811_int_gpio_mask(&hdl->dev, hdl->cfg.gpio_interrupt_mask);
    break;

  case STMPE811_HDL_STATE_SETUP_INT_GPIO_MASK:
    hdl->state = STMPE811_HDL_STATE_SETUP_INT_ADC_MASK;
    res = stmpe811_int_adc_mask(&hdl->dev, hdl->cfg.adc_interrupt_mask);
    break;

  case STMPE811_HDL_STATE_SETUP_INT_ADC_MASK:
    hdl->state = STMPE811_HDL_STATE_SETUP_INT_ENABLE;
    if (hdl->cfg.interrupt) {
      res = stmpe811_int_enable(&hdl->dev);
    } else {
      res = stmpe811_int_disable(&hdl->dev);
    }
    break;

  case STMPE811_HDL_STATE_SETUP_INT_ENABLE:
    hdl->state = STMPE811_HDL_STATE_SETUP_ADC_CFG;
    res = stmpe811_gpio_config_detect(&hdl->dev,
        hdl->cfg.gpio_detect_falling,
        hdl->cfg.gpio_detect_rising);
    break;

  case STMPE811_HDL_STATE_SETUP_ADC_CFG:
    hdl->state = STMPE811_HDL_STATE_SETUP_TEMP_CFG;
    res = stmpe811_adc_config(&hdl->dev,
        hdl->cfg.adc_clk,
        hdl->cfg.adc_time,
        hdl->cfg.adc_resolution,
        hdl->cfg.adc_reference);
    break;

  case STMPE811_HDL_STATE_SETUP_TEMP_CFG:
    hdl->state = STMPE811_HDL_STATE_SETUP_TEMP_THRES_CFG;
    res = stmpe811_temp_config(&hdl->dev,
        hdl->cfg.temp_enable,
        hdl->cfg.temp_mode);
    break;

  case STMPE811_HDL_STATE_SETUP_TEMP_THRES_CFG:
    hdl->state = STMPE811_HDL_STATE_SETUP_FINISHED;
    res = stmpe811_temp_config_threshold(&hdl->dev,
        hdl->cfg.temp_threshold_enable,
        hdl->cfg.temp_thres_mode,
        hdl->cfg.temp_threshold);
    break;

  case STMPE811_HDL_STATE_SETUP_FINISHED:
    hdl->state = STMPE811_HDL_STATE_IDLE;
    if (hdl->err_cb) hdl->err_cb(state, res);
    res = I2C_OK;
    break;

  default:
    res = I2C_ERR_STMPE811_UNKNOWN_STATE;
    break;
  }

  if (res != I2C_OK) {
    hdl->state = STMPE811_HDL_STATE_IDLE;
    if (hdl->err_cb) hdl->err_cb(state, res);
  }
}

void stmpe811_handler_open(stmpe811_handler *hdl, i2c_bus *bus,
    stmpe811_handler_gpio_cb_fn gpio_cb,
    stmpe811_handler_adc_cb_fn adc_cb,
    stmpe811_handler_temp_cb_fn temp_cb,
    stmpe811_handler_err_cb_fn err_cb) {
  memset(hdl, 0, sizeof(stmpe811_handler));
  hdl->gpio_cb = gpio_cb;
  hdl->adc_cb = adc_cb;
  hdl->temp_cb = temp_cb;
  hdl->err_cb = err_cb;
  stmpe811_open(&hdl->dev, bus, _stmpe811_cb);
  hdl->dev.i2c_stmpe.user_data = hdl;
}

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
    u16_t temp_threshold) {
  hdl->cfg.power_block = (power_block) & (STMPE_BLOCK_TEMP | STMPE_BLOCK_TSC | STMPE_BLOCK_GPIO | STMPE_BLOCK_ADC);
  hdl->cfg.gpio_analog_digital_mask = gpio_analog_digital_mask;
  hdl->cfg.gpio_direction = gpio_direction & gpio_analog_digital_mask;
  hdl->cfg.gpio_default_output = gpio_default_output & gpio_analog_digital_mask;

  hdl->cfg.interrupt = interrupt;
  hdl->cfg.interrupt_polarity = interrupt_polarity;
  hdl->cfg.interrupt_type = interrupt_type;

  hdl->cfg.interrupt_mask = interrupt_mask;
  hdl->cfg.gpio_interrupt_mask = gpio_interrupt_mask;
  hdl->cfg.adc_interrupt_mask = adc_interrupt_mask;

  hdl->cfg.gpio_detect_rising = gpio_detect_rising;
  hdl->cfg.gpio_detect_falling = gpio_detect_falling;

  hdl->cfg.adc_clk = adc_clk;
  hdl->cfg.adc_time = adc_time;
  hdl->cfg.adc_resolution = adc_resolution;
  hdl->cfg.adc_reference = adc_reference;

  hdl->cfg.temp_enable = temp_enable;
  hdl->cfg.temp_mode = temp_mode;
  hdl->cfg.temp_threshold_enable = temp_threshold_enable;
  hdl->cfg.temp_thres_mode = temp_thres_mode;
  hdl->cfg.temp_threshold = temp_threshold;

  hdl->state = STMPE811_HDL_STATE_SETUP_ID;

  return stmpe811_identify(&hdl->dev);
}

void stmpe811_handler_close(stmpe811_handler *hdl) {
  stmpe811_close(&hdl->dev);
}

int stmpe811_handler_interrupt_cb(stmpe811_handler *hdl) {
  hdl->state = STMPE811_HDL_STATE_INT_ENTRY;
  return stmpe811_int_status(&hdl->dev);
}

int stmpe811_handler_gpio_define(stmpe811_handler *hdl, u8_t set, u8_t reset) {
  hdl->state = STMPE811_HDL_STATE_GPIO_DEFINE_CLR;
  hdl->gpio_state = set;
  return stmpe811_gpio_clr(&hdl->dev, reset);
}

int stmpe811_handler_adc_read(stmpe811_handler *hdl, u8_t gpio_adc) {
  u8_t adc = 0;
  if (gpio_adc & STMPE_ADC7) adc |= STMPE_GPIO_ADC7;
  if (gpio_adc & STMPE_ADC6) adc |= STMPE_GPIO_ADC6;
  if (gpio_adc & STMPE_ADC5) adc |= STMPE_GPIO_ADC5;
  if (gpio_adc & STMPE_ADC4) adc |= STMPE_GPIO_ADC4;
  if (gpio_adc & STMPE_ADC3) adc |= STMPE_GPIO_ADC3;
  if (gpio_adc & STMPE_ADC2) adc |= STMPE_GPIO_ADC2;
  if (gpio_adc & STMPE_ADC1) adc |= STMPE_GPIO_ADC1;
  if (gpio_adc & STMPE_ADC0) adc |= STMPE_GPIO_ADC0;
  hdl->state = STMPE811_HDL_STATE_ADC_READ;
  hdl->adc_chan = adc;
  return stmpe811_adc_trigger(&hdl->dev, adc);
}

int stmpe811_handler_temp_read(stmpe811_handler *hdl, bool enable) {
  (void)enable; // todo
  hdl->state = STMPE811_HDL_STATE_TEMP_READ;
  return stmpe811_temp_acquire(&hdl->dev, TRUE);
}


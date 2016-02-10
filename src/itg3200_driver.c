/*
 * itg3200_driver.c
 *
 *  Created on: Dec 29, 2015
 *      Author: petera
 */

#include "itg3200_driver.h"

static void itg_cb(i2c_dev *idev, int res) {
  itg3200_dev *dev = (itg3200_dev *)I2C_DEV_get_user_data(idev);
  itg_state old_state = dev->state;
  if (res != I2C_OK) {
    dev->state = ITG3200_STATE_IDLE;
    if (dev->callback) dev->callback(dev, old_state, res);
    return;
  }

  switch (dev->state) {
  case ITG3200_STATE_CONFIG:
    break;

  case ITG3200_STATE_READ:
    if (dev->arg.data) {
      dev->arg.data->temp = (dev->tmp_buf[1] << 8) | dev->tmp_buf[2];
      dev->arg.data->x = (dev->tmp_buf[3] << 8) | dev->tmp_buf[4];
      dev->arg.data->y = (dev->tmp_buf[5] << 8) | dev->tmp_buf[6];
      dev->arg.data->z = (dev->tmp_buf[7] << 8) | dev->tmp_buf[8];
    }
    break;

  case ITG3200_STATE_READ_INTERRUPTS:
    if (dev->arg.int_status) {
      dev->arg.int_status->raw = dev->tmp_buf[1];
    }
    break;

  case ITG3200_STATE_ID:
    if (dev->arg.id_ok) {
      *dev->arg.id_ok = (dev->tmp_buf[1] & 0b01111110) == ITG3200_ID;
    }
    break;

  case ITG3200_STATE_IDLE:
    res = I2C_ERR_ITG3200_STATE;
    break;
  }

  dev->state = ITG3200_STATE_IDLE;

  if (dev->callback) dev->callback(dev, old_state, res);
}

void itg_open(itg3200_dev *dev, i2c_bus *bus, bool ad0, u32_t clock,
    void (*itg_callback)(itg3200_dev *dev, itg_state state, int res)) {
  memset(dev, 0, sizeof(itg3200_dev));
  I2C_DEV_init(&dev->i2c_dev, clock, bus, ad0 ? ITG3200_I2C_ADDR1 : ITG3200_I2C_ADDR0);
  I2C_DEV_set_user_data(&dev->i2c_dev, dev);
  I2C_DEV_set_callback(&dev->i2c_dev, itg_cb);
  I2C_DEV_open(&dev->i2c_dev);
  dev->callback = itg_callback;
}

void itg_close(itg3200_dev *dev) {
  I2C_DEV_close(&dev->i2c_dev);
}

int itg_config(itg3200_dev *dev, const itg_cfg *cfg) {

  dev->tmp_buf[0] = ITG3200_R_SMPLRT_DIV;
  dev->tmp_buf[1] = cfg->samplerate_div;
  dev->tmp_buf[2] = 0b00011000 | (cfg->lp_filter_rate & 7);
  dev->tmp_buf[3] = 0 |
      (cfg->int_act << 7) |
      (cfg->int_odpp << 6) |
      (cfg->int_latch_pulse << 5) |
      (cfg->int_clr << 4) |
      (cfg->int_pll << 2) |
      (cfg->int_data << 0);
  dev->tmp_buf[4] = ITG3200_R_PWR_MGM;
  dev->tmp_buf[5] = 0 |
      (cfg->pwr_reset << 7) |
      (cfg->pwr_sleep << 6) |
      (cfg->pwr_stdby_x << 5) |
      (cfg->pwr_stdby_y << 4) |
      (cfg->pwr_stdby_z << 3) |
      (cfg->pwr_clk & 0x7);

  I2C_SEQ_TX_STOP_C(dev->seq[0], &dev->tmp_buf[0], 4);
  I2C_SEQ_TX_STOP_C(dev->seq[1], &dev->tmp_buf[4], 2);

  dev->state = ITG3200_STATE_CONFIG;
  int res = I2C_DEV_sequence(&dev->i2c_dev, &dev->seq[0], 2);
  if (res != I2C_OK) {
    dev->state = ITG3200_STATE_IDLE;
  }

  return res;
}

int itg_read_data(itg3200_dev *dev, itg_reading *data) {
  if (data == NULL) {
    return I2C_ERR_ITG3200_NULLPTR;
  }

  dev->arg.data = data;
  dev->tmp_buf[0] = ITG3200_R_TEMP_OUT_H;
  I2C_SEQ_TX_C(dev->seq[0], &dev->tmp_buf[0], 1);
  I2C_SEQ_RX_STOP_C(dev->seq[1], &dev->tmp_buf[1], 8);
  dev->state = ITG3200_STATE_READ;
  int res = I2C_DEV_sequence(&dev->i2c_dev, &dev->seq[0], 2);
  if (res != I2C_OK) {
    dev->state = ITG3200_STATE_IDLE;
  }

  return res;
}

int itg_check_id(itg3200_dev *dev, bool *id_ok) {
  if (id_ok == NULL) {
    return I2C_ERR_ITG3200_NULLPTR;
  }

  dev->arg.id_ok = id_ok;
  dev->tmp_buf[0] = ITG3200_R_WHOAMI;
  I2C_SEQ_TX_C(dev->seq[0], &dev->tmp_buf[0], 1);
  I2C_SEQ_RX_STOP_C(dev->seq[1], &dev->tmp_buf[1], 1);
  dev->state = ITG3200_STATE_ID;
  int res = I2C_DEV_sequence(&dev->i2c_dev, &dev->seq[0], 2);
  if (res != I2C_OK) {
    dev->state = ITG3200_STATE_IDLE;
  }

  return res;
}

int itg_read_interrupts(itg3200_dev *dev, itg_int_status *int_sr) {
  if (int_sr == NULL) {
    return I2C_ERR_ITG3200_NULLPTR;
  }

  dev->arg.int_status = int_sr;
  dev->tmp_buf[0] = ITG3200_R_INT_STATUS;
  I2C_SEQ_TX_C(dev->seq[0], &dev->tmp_buf[0], 1);
  I2C_SEQ_RX_STOP_C(dev->seq[1], &dev->tmp_buf[1], 1);
  dev->state = ITG3200_STATE_READ_INTERRUPTS;
  int res = I2C_DEV_sequence(&dev->i2c_dev, &dev->seq[0], 2);
  if (res != I2C_OK) {
    dev->state = ITG3200_STATE_IDLE;
  }

  return res;
}


/////////////////////////////////////////////////////////////////////////// CLI

#ifndef CONFIG_CLI_ITG3200_OFF
#include "cli.h"
#include "miniutils.h"

static itg3200_dev itg_dev;
static itg_reading itg_data;
static itg_int_status itg_sr;
static bool itg_bool;

static void cli_itg_cb(itg3200_dev *dev, itg_state state, int res) {
  if (res < 0) print("itg_cb err %i\n", res);
  switch (state) {
  case ITG3200_STATE_CONFIG:
    print("itg cfg ok\n");
    break;
  case ITG3200_STATE_READ:
    // temp conv = raw_temp / 280 + 82.142857
    print("itg temp:%i (%i°C)  x:%i  y:%i  z:%i\n", itg_data.temp, itg_data.temp / 280 + 82, itg_data.x, itg_data.y, itg_data.z);
    break;
  case ITG3200_STATE_READ_INTERRUPTS:
    print("itg int:%s%s\n", itg_sr.data_ready ? " DATA_READY":"", itg_sr.pll_ready ? " PLL_READY":"");
    break;
  case ITG3200_STATE_ID:
    print("itg id ok: %s\n", itg_bool ? "TRUE":"FALSE");
    break;
  default:
    print("itg_cb unknown state %02x\n", state);
    break;
  }
}

static s32_t cli_itg_open(u32_t argc, u8_t bus, bool ad0, u32_t speed) {
  if (argc == 2) {
    speed = 100000;
  } else if (argc != 3) {
    return CLI_ERR_PARAM;
  }

  itg_open(&itg_dev, _I2C_BUS(bus), ad0, speed, cli_itg_cb);
  int res = itg_check_id(&itg_dev, &itg_bool);

  return res;
}

static s32_t cli_itg_close(u32_t argc) {
  itg_close(&itg_dev);
  return CLI_OK;
}

static s32_t cli_itg_cfg(u32_t argc) {
  itg_cfg cfg;
  cfg.samplerate_div = 0;
  cfg.pwr_clk = ITG3200_CLK_INTERNAL;
  cfg.pwr_reset = ITG3200_NO_RESET;
  cfg.pwr_sleep = ITG3200_ACTIVE;
  cfg.pwr_stdby_x = ITG3200_STNDBY_X_OFF;
  cfg.pwr_stdby_y = ITG3200_STNDBY_Y_OFF;
  cfg.pwr_stdby_z = ITG3200_STNDBY_Z_OFF;
  cfg.lp_filter_rate = ITG3200_LP_256;
  cfg.int_act = ITG3200_INT_ACTIVE_HI;
  cfg.int_clr = ITG3200_INT_CLR_SR;
  cfg.int_data = ITG3200_INT_WHEN_DATA;
  cfg.int_pll = ITG3200_INT_NO_PLL;
  cfg.int_latch_pulse = ITG3200_INT_LATCH;
  cfg.int_odpp = ITG3200_INT_PUSHPULL;
  int res = itg_config(&itg_dev, &cfg);
  return res;
}

static s32_t cli_itg_read(u32_t argc) {
  int res = itg_read_data(&itg_dev, &itg_data);
  return res;
}

static s32_t cli_itg_stat(u32_t argc) {
  int res = itg_read_interrupts(&itg_dev, &itg_sr);
  return res;
}

CLI_MENU_START(itg3200)
CLI_FUNC("cfg", cli_itg_cfg, "Configures itg3200 device\n"
        "cfg (TODO)\n"
        "ex: cfg\n")
CLI_FUNC("close", cli_itg_close, "Closes itg3200 device")
CLI_FUNC("open", cli_itg_open, "Opens itg3200 device\n"
        "open <bus> <ad0> (<bus_speed>)\n"
        "ex: open 0 0 100000\n")
CLI_FUNC("rd", cli_itg_read, "Reads gyroscope values")
CLI_FUNC("stat", cli_itg_stat, "Reads itg3200 data ready status")
CLI_MENU_END


#endif



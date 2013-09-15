/*
 * i2c_driver.h
 *
 *  Created on: Jul 8, 2013
 *      Author: petera
 */

#ifndef I2C_DRIVER_H_
#define I2C_DRIVER_H_

#include "system.h"

#define I2C_OK                0
#define I2C_RX_OK             1
#define I2C_TX_OK             2
#define I2C_ERR_BUS_BUSY      -1
#define I2C_ERR_UNKNOWN_STATE -2
#define I2C_ERR_PHY           -3

typedef enum {
  I2C_S_IDLE = 0,
  I2C_S_GEN_START,
  I2C_S_RX,
  I2C_S_TX,
} i2c_state;

typedef enum {
  I2C_ERR_PHY_SMBUS_ALERT = 0,
  I2C_ERR_PHY_TIMEOUT,
  I2C_ERR_PHY_ARBITRATION_LOST,
  I2C_ERR_PHY_PEC_ERR,
  I2C_ERR_PHY_OVER_UNDERRUN,
  I2C_ERR_PHY_ACK_FAIL,
  I2C_ERR_PHY_BUS_ERR
} i2c_phy_err;

/*
 * I2C bus
 */
typedef struct i2c_bus_s {
  void *hw;

  u8_t attached_devices;

  volatile i2c_state state;
  void (* i2c_bus_callback)(struct i2c_bus_s *s, int res);

  u8_t addr; // lsb denotes tx (0) or rx (1)
  u8_t *buf;
  u16_t len;
  bool gen_stop;

  void *user_p;
  volatile u32_t user_arg;
  u32_t bad_ev_counter;
  u32_t phy_error;
} i2c_bus;

extern i2c_bus __i2c_bus_vec[I2C_MAX_ID];

#define _I2C_BUS(x) (&__i2c_bus_vec[(x)])

void I2C_init();

void I2C_register(i2c_bus *bus);

int I2C_config(i2c_bus *bus, u32_t clock);

int I2C_query(i2c_bus *bus, u8_t addr);

int I2C_set_callback(i2c_bus *i, void (*i2c_bus_callback)(i2c_bus *i, int res));

int I2C_rx(i2c_bus *bus, u8_t addr, u8_t *rx, u16_t len, bool gen_stop);

int I2C_tx(i2c_bus *bus, u8_t addr, const u8_t *tx, u16_t len, bool gen_stop);

void I2C_release(i2c_bus *bus);

int I2C_close(i2c_bus *bus);

bool I2C_is_busy(i2c_bus *bus);

void I2C_reset(i2c_bus *bus);

u32_t I2C_phy_err(i2c_bus *bus);

void I2C_IRQ_err(i2c_bus *bus);

void I2C_IRQ_ev(i2c_bus *bus);

#endif /* I2C_DRIVER_H_ */

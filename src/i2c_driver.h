/*
 * i2c_driver.h
 *
 *  Created on: Jul 8, 2013
 *      Author: petera
 */

#ifndef I2C_DRIVER_H_
#define I2C_DRIVER_H_

#include "system.h"
#include "taskq.h"

//#define I2C_EV_LOG_DBG

#ifdef I2C_EV_LOG_DBG
#ifndef I2C_EV_LOG_LEN
#define I2C_EV_LOG_LEN 32
#endif
#define I2C_LOG_RESTART(_bus) _bus->log_ix = 0
#define I2C_LOG(_bus, _ev) if (_bus->log_ix < I2C_EV_LOG_LEN) _bus->log[_bus->log_ix++] = _ev
#else
#define I2C_LOG_RESTART(_bus)
#define I2C_LOG(_bus, _ev)
#endif

#define I2C_OK                0
#define I2C_RX_OK             1
#define I2C_TX_OK             2
#define I2C_ERR_BUS_BUSY      -1000
#define I2C_ERR_UNKNOWN_STATE -1001
#define I2C_ERR_PHY           -1002

typedef enum {
  I2C_S_IDLE = 0,
  I2C_S_GEN_START,
  I2C_S_RX,
  I2C_S_TX,
} i2c_state;

typedef enum {
  I2C_OP_RX = 0,
  I2C_OP_TX,
  I2C_OP_QUERY
} i2c_op;

typedef enum {
  I2C_ERR_PHY_SMBUS_ALERT = 0,      // 1
  I2C_ERR_PHY_TIMEOUT,              // 2
  I2C_ERR_PHY_ARBITRATION_LOST,     // 4
  I2C_ERR_PHY_PEC_ERR,              // 8
  I2C_ERR_PHY_OVER_UNDERRUN,        // 16
  I2C_ERR_PHY_ACK_FAIL,             // 32
  I2C_ERR_PHY_BUS_ERR               // 64
} i2c_phy_err;

/*
 * I2C bus
 */
typedef struct i2c_bus_s {
  void *hw;

  u8_t attached_devices;

  volatile i2c_state state;
  volatile i2c_op op;
  void (* i2c_bus_callback)(struct i2c_bus_s *s, int res);

  u8_t addr; // lsb denotes tx (0) or rx (1)
  u8_t *buf;
  u16_t len;
  bool gen_stop;
  volatile bool restart_generated;

  void *user_p;
  volatile u32_t user_arg;
  u32_t bad_ev_counter;
  u32_t phy_error;
#ifdef I2C_EV_LOG_DBG
  u32_t log[I2C_EV_LOG_LEN];
  volatile u8_t log_ix;
#endif
  task_mutex mutex;
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

void I2C_log_dump(i2c_bus *bus);

#define I2C_LOG_RX(bus, addr) I2C_LOG(bus, (0x12340000 | addr))
#define I2C_LOG_TX(bus, addr) I2C_LOG(bus, (0x12350000 | addr))
#define I2C_LOG_QUERY(bus, addr) I2C_LOG(bus, (0x12360000 | addr))
#define I2C_LOG_START(bus) I2C_LOG(bus, 0x01010000)
#define I2C_LOG_STOP(bus) I2C_LOG(bus, 0x01020000)
#define I2C_LOG_ERR(bus, err) I2C_LOG(bus, 0xffff0000 | err)
#define I2C_LOG_EV(bus, ev) I2C_LOG(bus, 0x80000000 | ev)
#define I2C_LOG_UNKNOWN_EV(bus, ev) I2C_LOG(bus, 0xf0000000 | ev)
#define I2C_LOG_CB(bus) I2C_LOG(bus, 0x43210000)

#endif /* I2C_DRIVER_H_ */

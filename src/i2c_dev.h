/*
 * i2c_dev.h
 *
 *  Created on: Aug 7, 2013
 *      Author: petera
 */

#ifndef I2C_DEV_H_
#define I2C_DEV_H_

#include "i2c_driver.h"
#include "taskq.h"

#define I2C_ERR_DEV_BUSY        -1010
#define I2C_ERR_DEV_TIMEOUT     -1011

#define I2C_SEQ_TX(buffer, length) \
  (i2c_dev_sequence){ \
  .dir = I2C_DEV_TX,    \
  .gen_stop = I2C_DEV_RESTART, \
  .len = (length), \
  .buf = buffer }

#define I2C_SEQ_RX(buffer, length) \
  (i2c_dev_sequence){ \
  .dir = I2C_DEV_RX,    \
  .gen_stop = I2C_DEV_RESTART, \
  .len = (length), \
  .buf = buffer }

#define I2C_SEQ_TX_STOP(buffer, length) \
  (i2c_dev_sequence){ \
  .dir = I2C_DEV_TX,    \
  .gen_stop = I2C_DEV_STOP, \
  .len = (length), \
  .buf = buffer }

#define I2C_SEQ_RX_STOP(buffer, length) \
  (i2c_dev_sequence){ \
  .dir = I2C_DEV_RX,    \
  .gen_stop = I2C_DEV_STOP, \
  .len = (length), \
  .buf = buffer }

typedef enum {
  I2C_DEV_RX = 0,
  I2C_DEV_TX
} i2c_dev_dir;

typedef enum {
  I2C_DEV_RESTART = 0,
  I2C_DEV_STOP
} i2c_dev_stop;

typedef struct  {
  i2c_dev_dir dir : 1;
  i2c_dev_stop gen_stop: 1;
  u32_t len : 30;
  const u8_t *buf;
} __attribute__(( packed )) i2c_dev_sequence;

/*
 * I2C device
 */
typedef struct i2c_dev_s {
  u32_t clock_configuration;
  i2c_bus *bus;
  task *tmo_task;
  task_timer tmo_tim;
  bool opened;
  u8_t addr;
  void (*i2c_dev_callback)(struct i2c_dev_s *s, int result);
  i2c_dev_sequence cur_seq;
  i2c_dev_sequence *seq_list;
  u8_t seq_len;
  volatile void *user_data;
} i2c_dev;

typedef void (*i2c_dev_callback)(i2c_dev *dev, int result);

void I2C_DEV_init(i2c_dev *dev, u32_t clock, i2c_bus *bus, u8_t addr);

void I2C_DEV_open(i2c_dev *dev);

int I2C_DEV_set_callback(i2c_dev *dev, i2c_dev_callback cb);

void I2C_DEV_set_user_data(i2c_dev *dev, volatile void *user_data);

volatile void *I2C_DEV_get_user_data(i2c_dev *dev);

int I2C_DEV_sequence(i2c_dev *dev, const i2c_dev_sequence *seq, u8_t seq_len);

void I2C_DEV_close(i2c_dev *dev);

bool I2C_DEV_is_busy(i2c_dev *dev);

#endif /* I2C_DEV_H_ */

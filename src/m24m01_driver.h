/*
 * m24m01_driver.h
 *
 *  Created on: Jan 13, 2014
 *      Author: petera
 */

#ifndef M24M01_DRIVER_H_
#define M24M01_DRIVER_H_

#include "system.h"
#include "i2c_dev.h"
#include "taskq.h"


#define I2C_ERR_M24M01_ADDRESS_RANGE_BAD -1201
#define I2C_ERR_M24M01_BAD_STATE         -1202
#define I2C_ERR_M24M01_UNRESPONSIVE      -1203

typedef enum {
  M24M01_IDLE = 0,
  M24M01_READ,
  M24M01_WRITE,
} m24m01_state;

typedef struct m24m01_dev_s {
  bool open;
  i2c_dev i2c_h;
  i2c_dev i2c_l;
  void (*callback)(struct m24m01_dev_s *dev, int res);
  u8_t dev_addr;

  volatile m24m01_state state;
  u32_t addr;
  u8_t *data;
  u32_t len;
  u32_t oplen;
  u8_t query_tries;

  i2c_dev_sequence seq[2];
  u8_t tmp[2+256];

  task *retry_task;
  task_timer retry_timer;

} m24m01_dev;

void m24m01_open(m24m01_dev *dev, i2c_bus *bus, bool e1, bool e2,
    u32_t bus_speed, void (*m24m01_callback)(m24m01_dev *dev, int res));
void m24m01_close(m24m01_dev *dev);
int m24m01_read(m24m01_dev *dev, u32_t addr, u8_t *buf, u32_t len);
int m24m01_write(m24m01_dev *dev, u32_t addr, u8_t *buf, u32_t len);

#endif /* M24M01_DRIVER_H_ */

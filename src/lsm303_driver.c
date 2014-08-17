/*
 * lsm303_driver.c
 *
 *  Created on: Aug 24, 2013
 *      Author: petera
 */

#include "lsm303_driver.h"
#include "trig_q.h"

#define LSM_ACC_ADDR_SA00       0x30
#define LSM_ACC_ADDR_SA01       0x32
#define LSM_MAG_ADDR            0x3c

#define LSM_CTRL_REG_1_A        0x20
#define LSM_CTRL_REG_2_A        0x21
#define LSM_CTRL_REG_3_A        0x22
#define LSM_CTRL_REG_4_A        0x23
#define LSM_CTRL_REG_5_A        0x24
#define LSM_HP_FILTER_RESET_A   0x25
#define LSM_REFERENCE_A         0x26
#define LSM_STATUS_REG_A        0x27
#define LSM_OUT_X_L_A           0x28
#define LSM_OUT_X_H_A           0x29
#define LSM_OUT_Y_L_A           0x2a
#define LSM_OUT_Y_H_A           0x2b
#define LSM_OUT_Z_L_A           0x2c
#define LSM_OUT_Z_H_A           0x2d
#define LSM_INT1_CFG_A          0x30
#define LSM_INT1_SOURCE_A       0x31
#define LSM_INT1_THS_A          0x32
#define LSM_INT1_DURATION_A     0x33
#define LSM_INT2_CFG_A          0x34
#define LSM_INT2_SOURCE_A       0x35
#define LSM_INT2_THS_A          0x36
#define LSM_INT2_DURATION_A     0x37

#define LSM_CRA_REG_M           0x00
#define LSM_CRB_REG_M           0x01
#define LSM_MR_REG_M            0x02
#define LSM_OUT_X_H_M           0x03
#define LSM_OUT_X_L_M           0x04
#define LSM_OUT_Y_H_M           0x05
#define LSM_OUT_Y_L_M           0x06
#define LSM_OUT_Z_H_M           0x07
#define LSM_OUT_Z_L_M           0x08
#define LSM_SR_REG_M            0x09
#define LSM_IRA_REG_M           0x0a
#define LSM_IRB_REG_M           0x0b
#define LSM_IRC_REG_M           0x0c

#define LSM_MAX_VAL_M           (s16_t)(0x400-1)
#define LSM_MIN_VAL_M           (s16_t)(0-0x400)

// enable accelerometer, LSM_CTRL_REG_1_A = 0b00100111
// enable all axes, normal power
static const u8_t LSM_ACC_INIT[] = {LSM_CTRL_REG_1_A, 0x27};

// enable magnetometer, LSM_MR_REG_M = 0b00000000
// continuous conversion
static const u8_t LSM_MAG_INIT[] = {LSM_MR_REG_M, 0x00};

static const i2c_dev_sequence lsm_seq_acc_init[] = {
    I2C_SEQ_TX_STOP(LSM_ACC_INIT, sizeof(LSM_ACC_INIT))
};

static const i2c_dev_sequence lsm_seq_mag_init[] = {
    I2C_SEQ_TX_STOP(LSM_MAG_INIT, sizeof(LSM_MAG_INIT))
};


static void lsm_acc_cb(i2c_dev *idev, int res) {
  lsm303_dev *dev = (lsm303_dev *)I2C_DEV_get_user_data(idev);
  if (res != I2C_OK) {
    dev->state = LSM_STATE_ERROR;
    if (dev->callback) dev->callback(dev, res);
    return;
  }
  switch (dev->state) {
  case LSM_STATE_INIT:
    res = I2C_DEV_sequence(&dev->i2c_mag, lsm_seq_mag_init, 1);
    break;
  case LSM_STATE_READ_BOTH:
  case LSM_STATE_READ_ACC: {
    bool call_mag = dev->state == LSM_STATE_READ_BOTH;
    dev->state = LSM_STATE_IDLE;
    s16_t x, y, z;
    x = (dev->buf[0] | ((s8_t)dev->buf[1] << 8))>>4;
    y = (dev->buf[2] | ((s8_t)dev->buf[3] << 8))>>4;
    z = (dev->buf[4] | ((s8_t)dev->buf[5] << 8))>>4;
    u8_t lp = dev->lp_filter_acc;
    if (lp > 0) {
      // (x*(256-k) + y*k)/256 = (256*x - x*k + y*k)/256 = (256*x - k*(x - y))/256
      dev->acc[0] = (s16_t)( ((s32_t)x*256 - (s32_t)lp*((s32_t)x - (s32_t)dev->acc[0]))>>8 );
      dev->acc[1] = (s16_t)( ((s32_t)y*256 - (s32_t)lp*((s32_t)y - (s32_t)dev->acc[1]))>>8 );
      dev->acc[2] = (s16_t)( ((s32_t)z*256 - (s32_t)lp*((s32_t)z - (s32_t)dev->acc[2]))>>8 );
    } else {
      dev->acc[0] = x;
      dev->acc[1] = y;
      dev->acc[2] = z;
    }
    if (call_mag) {
      res = lsm_read_mag(dev);
    } else {
      if (dev->callback) dev->callback(dev, res);
    }
    break;
  }
  default:
    res = I2C_ERR_LSM303_STATE;
    break;
  }
  if (res != I2C_OK) {
    dev->state = LSM_STATE_ERROR;
    if (dev->callback) dev->callback(dev, res);
  }
}

static void lsm_mag_cb(i2c_dev *idev, int res) {
  lsm303_dev *dev = (lsm303_dev *)I2C_DEV_get_user_data(idev);
  if (res != I2C_OK) {
    dev->state = LSM_STATE_ERROR;
    if (dev->callback) dev->callback(dev, res);
    return;
  }
  switch (dev->state) {
  case LSM_STATE_INIT:
    dev->state = LSM_STATE_IDLE;
    if (dev->callback) dev->callback(dev, res);
    break;
  case LSM_STATE_READ_MAG: {
    dev->state = LSM_STATE_IDLE;
    s16_t x, y, z;
    x = dev->buf[1] | (dev->buf[0] << 8);
    y = dev->buf[3] | (dev->buf[2] << 8);
    z = dev->buf[5] | (dev->buf[4] << 8);
    if (x < LSM_MAX_VAL_M && x > LSM_MIN_VAL_M &&
        y < LSM_MAX_VAL_M && y > LSM_MIN_VAL_M &&
        z < LSM_MAX_VAL_M && z > LSM_MIN_VAL_M) {
      u8_t lp = dev->lp_filter_mag;
      if (lp > 0) {
        // (x*(256-k) + y*k)/256 = (256*x - x*k + y*k)/256 = (256*x - k*(x - y))/256
        dev->mag[0] = (s16_t)( ((s32_t)x*256 - (s32_t)lp*((s32_t)x - (s32_t)dev->mag[0]))>>8 );
        dev->mag[1] = (s16_t)( ((s32_t)y*256 - (s32_t)lp*((s32_t)y - (s32_t)dev->mag[1]))>>8 );
        dev->mag[2] = (s16_t)( ((s32_t)z*256 - (s32_t)lp*((s32_t)z - (s32_t)dev->mag[2]))>>8 );
      } else {
        dev->mag[0] = x;
        dev->mag[1] = y;
        dev->mag[2] = z;
      }
      if (dev->callback) dev->callback(dev, res);
    } else {
      res = I2C_ERR_LSM303_BAD_READ;
    }
    break;
  }
  default:
    res = I2C_ERR_LSM303_STATE;
    break;
  }
  if (res != I2C_OK) {
    dev->state = LSM_STATE_ERROR;
    if (dev->callback) dev->callback(dev, res);
    return;
  }
}

void lsm_open(lsm303_dev *dev, i2c_bus *bus, bool sa0, void (*lsm_callback)(lsm303_dev *dev, int res)) {
  const u32_t clock = 50000;
  I2C_DEV_init(&dev->i2c_acc, clock, bus, sa0 ? LSM_ACC_ADDR_SA01 : LSM_ACC_ADDR_SA00);
  I2C_DEV_init(&dev->i2c_mag, clock, bus, LSM_MAG_ADDR);
  I2C_DEV_set_user_data(&dev->i2c_acc, dev);
  I2C_DEV_set_user_data(&dev->i2c_mag, dev);
  I2C_DEV_set_callback(&dev->i2c_acc, lsm_acc_cb);
  I2C_DEV_set_callback(&dev->i2c_mag, lsm_mag_cb);
  I2C_DEV_open(&dev->i2c_acc);
  I2C_DEV_open(&dev->i2c_mag);
  dev->lp_filter_acc = 0;
  dev->lp_filter_mag = 0;
  memset(dev->acc, 0, sizeof(dev->acc));
  memset(dev->mag, 0, sizeof(dev->mag));
  memset(dev->mag_limit_max, 0, sizeof(dev->mag_limit_max));
  memset(dev->mag_limit_min, 0, sizeof(dev->mag_limit_min));

  dev->acc_limit_min[0] = -1064;
  dev->acc_limit_max[0] = 1051;
  dev->acc_limit_min[1] = -1003;
  dev->acc_limit_max[1] = 1041;
  dev->acc_limit_min[2] = -1012;
  dev->acc_limit_max[2] = 1143;

  dev->mag_limit_min[0] = -683;
  dev->mag_limit_max[0] = 380;
  dev->mag_limit_min[1] = -534;
  dev->mag_limit_max[1] = 550;
  dev->mag_limit_min[2] = -575;
  dev->mag_limit_max[2] = 453;

  /*
  -683 < x < 380
-534 < y < 550
-572 < z < 453

   */

  dev->callback = lsm_callback;
  dev->state = LSM_STATE_IDLE;
}

void lsm_close(lsm303_dev *dev) {
  I2C_DEV_close(&dev->i2c_acc);
  I2C_DEV_close(&dev->i2c_mag);
}

int lsm_config_default(lsm303_dev *dev) {
  if (dev->state != LSM_STATE_IDLE && dev->state != LSM_STATE_ERROR) {
    return I2C_ERR_DEV_BUSY;
  }
  int res;
  dev->state = LSM_STATE_INIT;
  res = I2C_DEV_sequence(&dev->i2c_acc, &lsm_seq_acc_init[0], 1);

  if (res != I2C_OK) {
    dev->state = LSM_STATE_ERROR;
  }

  return res;
}

void lsm_set_acc_limits(lsm303_dev *dev, s16_t min[3], s16_t max[3]) {
  memcpy(dev->acc_limit_min, min, sizeof(dev->acc_limit_min));
  memcpy(dev->acc_limit_max, max, sizeof(dev->acc_limit_max));
}

void lsm_set_mag_limits(lsm303_dev *dev, s16_t min[3], s16_t max[3]) {
  memcpy(dev->mag_limit_min, min, sizeof(dev->mag_limit_min));
  memcpy(dev->mag_limit_max, max, sizeof(dev->mag_limit_max));
}

static int lsm_read_acc_s(lsm303_dev *dev, enum lsm_state state) {
  if (dev->state != LSM_STATE_IDLE && dev->state != LSM_STATE_ERROR) {
    return I2C_ERR_DEV_BUSY;
  }
  int res;

  dev->state = state;

  dev->buf[0] = LSM_OUT_X_L_A | (1<<7); // set msb acc to datasheet to have autoinc reg

  dev->seq[0].dir = I2C_DEV_TX;
  dev->seq[0].buf = &dev->buf[0];
  dev->seq[0].len = 1;
  dev->seq[0].gen_stop = I2C_DEV_RESTART;

  dev->seq[1].dir = I2C_DEV_RX;
  dev->seq[1].buf = &dev->buf[0];
  dev->seq[1].len = 6;
  dev->seq[1].gen_stop = I2C_DEV_STOP;

  res = I2C_DEV_sequence(&dev->i2c_acc, &dev->seq[0], 2);

  if (res != I2C_OK) {
    dev->state = LSM_STATE_ERROR;
  }

  return res;
}

int lsm_read_acc(lsm303_dev *dev) {
  return lsm_read_acc_s(dev, LSM_STATE_READ_ACC);
}

int lsm_read_both(lsm303_dev *dev) {
  return lsm_read_acc_s(dev, LSM_STATE_READ_BOTH);
}

int lsm_read_mag(lsm303_dev *dev) {
  if (dev->state != LSM_STATE_IDLE && dev->state != LSM_STATE_ERROR) {
    return I2C_ERR_DEV_BUSY;
  }
  int res;

  dev->state = LSM_STATE_READ_MAG;

  dev->buf[0] = LSM_OUT_X_H_M;

  dev->seq[0].dir = I2C_DEV_TX;
  dev->seq[0].buf = &dev->buf[0];
  dev->seq[0].len = 1;
  dev->seq[0].gen_stop = I2C_DEV_RESTART;

  dev->seq[1].dir = I2C_DEV_RX;
  dev->seq[1].buf = &dev->buf[0];
  dev->seq[1].len = 6;
  dev->seq[1].gen_stop = I2C_DEV_STOP;

  res = I2C_DEV_sequence(&dev->i2c_mag, &dev->seq[0], 2);

  if (res != I2C_OK) {
    dev->state = LSM_STATE_ERROR;
  }

  return res;
}

s16_t *lsm_get_acc_reading(lsm303_dev *dev) {
  return dev->acc;
}

s16_t *lsm_get_mag_reading(lsm303_dev *dev) {
  return dev->mag;
}

static void _lsm_cross(s16_t a[3], s16_t b[3], s16_t r[3]) {
  s32_t t;
  t = (s32_t)a[1]*(s32_t)b[2] - (s32_t)a[2]*(s32_t)b[1];
  r[0] = t>>15;
  t = (s32_t)a[2]*(s32_t)b[0] - (s32_t)a[0]*(s32_t)b[2];
  r[1] = t>>15;
  t = (s32_t)a[0]*(s32_t)b[1] - (s32_t)a[1]*(s32_t)b[0];
  r[2] = t>>15;
}

static s32_t _lsm_dot(s16_t a[3], s16_t b[3]) {
  s32_t r;
  r = (s32_t)a[0]*(s32_t)b[0] + (s32_t)a[1]*(s32_t)b[1] + (s32_t)a[2]*(s32_t)b[2];
  return r>>15;
}

static void _lsm_norm(s16_t a[3], s16_t r[3]) {
  s16_t x,y,z;
  x = a[0];
  y = a[1];
  z = a[2];
  s32_t x2 = x*x;
  s32_t y2 = y*y;
  s32_t z2 = z*z;
  s32_t d = _sqrt(x2+y2+z2);
  s32_t xn = (x<<15)/d;
  s32_t yn = (y<<15)/d;
  s32_t zn = (z<<15)/d;

  r[0] = xn;
  r[1] = yn;
  r[2] = zn;
}

void lsm_set_lowpass(lsm303_dev *dev, u8_t acc, u8_t mag) {
  dev->lp_filter_acc = acc;
  dev->lp_filter_mag = mag;
}

u16_t lsm_get_heading(lsm303_dev *dev) {
  int i;
  // Q15

  s16_t m_scaled[3];
  s16_t a_scaled[3];
  s16_t up[3] = {0, -(1<<15), 0};

  // scale and adjust mag vector acc to prefound max/mins
  for (i = 0; i < 3; i++) {
    m_scaled[i] = ((2*(dev->mag[i] - dev->mag_limit_min[i])) << 15) /
        (dev->mag_limit_max[i] - dev->mag_limit_min[i]) + (1<<15);
  }
  // scale and adjust accele vector acc to prefound max/mins
  for (i = 0; i < 3; i++) {
    a_scaled[i] = ((2*(dev->acc[i] - dev->acc_limit_min[i])) << 15) /
        (dev->acc_limit_max[i] - dev->acc_limit_min[i]) + (1<<15);
  }

  s16_t a_norm[3];
  s16_t e[3];
  s16_t n[3];
  // normalize acc vector
  _lsm_norm(a_scaled, a_norm);
  // get east vector by crossing mag and acc
  _lsm_cross(m_scaled, a_norm, e);
  // normalize it
  _lsm_norm(e, e);
  // get north vector by crossing acc and east
  _lsm_cross(a_norm, e, n);

  // dot out y and x for atan2 (this could be optimized, see up vec)
  s16_t yy = _lsm_dot(n, up);
  s16_t xx = _lsm_dot(e, up);

  return atan2_approx(yy, xx);
}

/*
 * m24m01_driver.c
 *
 *  Created on: Jan 13, 2014
 *      Author: petera
 */

#include "m24m01_driver.h"
#include "miniutils.h"
#include <stdarg.h>

/*
 * When M24M01 is busy, it does not ack its address.
 *
 * Hence reads and writes accept a number of I2C_ERR_PHY
 * when bit I2C_ERR_PHY_ACK_FAIL is set in I2C_phy_err.
 */

#define M24M01_SIZE         (128*1024)
#define M24M01_HIGH_START   (M24M01_SIZE/2)
#define M24M01_MAX_QUERIES  8

//#define EEDBG(x, ...) print("M24M01:"x, ## __VA_ARGS__)
#define EEDBG(x, ...)

static int _m24m01_read(m24m01_dev *dev, u32_t addr, u8_t *buf, u32_t len);

static int _m24m01_write(m24m01_dev *dev, u32_t addr, u8_t *buf, u32_t len);

static void m24m01_retry_task_f(u32_t arg, void *argp) {
  m24m01_dev *dev = (m24m01_dev *)argp;
  int res;
  if (!dev->open) return;
  if (dev->state == M24M01_READ) {
    res = _m24m01_read(dev, dev->addr, dev->data, dev->len);
  } else if (dev->state == M24M01_WRITE) {
    res = _m24m01_write(dev, dev->addr, dev->data, dev->len);
  } else {
    res = I2C_ERR_M24M01_BAD_STATE;
  }
  if (res != I2C_OK) {
    // error on retry
    EEDBG("error on retry %i\n", res);
    dev->state = M24M01_IDLE;
    if (dev->callback) dev->callback(dev, res);
    return;
  }
}

static void m24m01_retry(m24m01_dev *dev) {
  dev->retry_task = TASK_create(m24m01_retry_task_f, 0);
  ASSERT(dev->retry_task);
  TASK_start_timer(dev->retry_task, &dev->retry_timer,
      0, dev, 5, 0, "m24m01_retry");
}

static void m24m01_cb(i2c_dev *idev, int res) {
  m24m01_dev *dev = (m24m01_dev *)I2C_DEV_get_user_data(idev);

  EEDBG("cb state %i, res %i\n", dev->state, res);
  if (res == I2C_ERR_PHY) {
    EEDBG("i2c phy err %08b\n", I2C_phy_err(idev->bus));
  }

  if (res < I2C_OK) {
    // error
    if (res == I2C_ERR_PHY && (I2C_phy_err(idev->bus) & (1<<I2C_ERR_PHY_ACK_FAIL))) {
      // no ack, m24m01 might be busy
      EEDBG("no ack!\n");
      dev->query_tries++;
      if (dev->query_tries < M24M01_MAX_QUERIES) {
        EEDBG("retry %i\n", dev->query_tries);
        m24m01_retry(dev);
        return;
      }
      // else continue, report fail and bail out
    }
    dev->state = M24M01_IDLE;
    if (dev->callback) dev->callback(dev, res);
    return;
  }

  if (dev->state == M24M01_READ || dev->state == M24M01_WRITE) {
    dev->addr += dev->oplen;
    dev->data += dev->oplen;
    dev->len -= dev->oplen;
  }

  if (dev->len == 0) {
    // finished
    dev->state = M24M01_IDLE;
    if (dev->callback) dev->callback(dev, I2C_OK);
    return;
  }

  if (dev->state == M24M01_READ) {
    res = _m24m01_read(dev, dev->addr, dev->data, dev->len);
  } else if (dev->state == M24M01_WRITE) {
    res = _m24m01_write(dev, dev->addr, dev->data, dev->len);
  } else {
    res = I2C_ERR_M24M01_BAD_STATE;
  }

  if (res != I2C_OK) {
    // error on next op
    dev->state = M24M01_IDLE;
    if (dev->callback) dev->callback(dev, res);
    return;
  }
}

void m24m01_open(m24m01_dev *dev, i2c_bus *bus, bool e1, bool e2, u32_t bus_speed,
    void (*m24m01_callback)(m24m01_dev *dev, int res)) {
  memset(dev, 0, sizeof(m24m01_dev));
  dev->state = M24M01_IDLE;
  dev->callback = m24m01_callback;
  dev->dev_addr = 0b10100000;
  dev->dev_addr |= e2 ? (0b10000) : 0;
  dev->dev_addr |= e1 ? (0b01000) : 0;
  I2C_DEV_init(&dev->i2c_h, bus_speed, bus, dev->dev_addr | 0b10);
  I2C_DEV_init(&dev->i2c_l, bus_speed, bus, dev->dev_addr);
  I2C_DEV_set_user_data(&dev->i2c_h, dev);
  I2C_DEV_set_user_data(&dev->i2c_l, dev);
  I2C_DEV_set_callback(&dev->i2c_h, m24m01_cb);
  I2C_DEV_set_callback(&dev->i2c_l, m24m01_cb);
  I2C_DEV_open(&dev->i2c_h);
  I2C_DEV_open(&dev->i2c_l);
  dev->open = TRUE;
}

void m24m01_close(m24m01_dev *dev) {
  dev->open = FALSE;
  I2C_DEV_close(&dev->i2c_h);
  I2C_DEV_close(&dev->i2c_l);
}

static int _m24m01_read(m24m01_dev *dev, u32_t addr, u8_t *buf, u32_t len) {
  if (addr > M24M01_SIZE || (addr+len) > M24M01_SIZE) {
    return I2C_ERR_M24M01_ADDRESS_RANGE_BAD;
  }

  int res = I2C_OK;

  i2c_dev *adev;
  if (addr >= M24M01_HIGH_START) {
    addr -= M24M01_HIGH_START;
    adev = &dev->i2c_h;
    dev->oplen = len;
  } else {
    adev = &dev->i2c_l;
    dev->oplen = MIN(M24M01_HIGH_START - addr, len);
  }

  dev->tmp[0] = addr >> 8;
  dev->tmp[1] = addr & 0xff;

  dev->seq[0].dir = I2C_DEV_TX;
  dev->seq[0].buf = &dev->tmp[0];
  dev->seq[0].len = 2;
  dev->seq[0].gen_stop = I2C_DEV_RESTART;

  dev->seq[1].dir = I2C_DEV_RX;
  dev->seq[1].buf = buf;
  dev->seq[1].len = dev->oplen;
  dev->seq[1].gen_stop = I2C_DEV_STOP;

  EEDBG("read, sequence\n");
  res = I2C_DEV_sequence(adev, &dev->seq[0], 2);

  return res;
}

int m24m01_read(m24m01_dev *dev, u32_t addr, u8_t *buf, u32_t len) {
  if (addr > M24M01_SIZE || (addr+len) > M24M01_SIZE) {
    return I2C_ERR_M24M01_ADDRESS_RANGE_BAD;
  }
  if (dev->state != M24M01_IDLE) {
    return I2C_ERR_DEV_BUSY;
  }

  dev->addr = addr;
  dev->data = buf;
  dev->len = len;
  dev->oplen = 0;
  dev->state = M24M01_READ;
  dev->query_tries = 0;

  int res = _m24m01_read(dev, addr, buf, len);

  if (res < I2C_OK) {
    dev->state = M24M01_IDLE;
  }

  return res;
}

static int _m24m01_write(m24m01_dev *dev, u32_t addr, u8_t *buf, u32_t len) {
  if (addr > M24M01_SIZE || (addr+len) > M24M01_SIZE) {
    return I2C_ERR_M24M01_ADDRESS_RANGE_BAD;
  }

  int res = I2C_OK;

  i2c_dev *adev;
  if (addr >= M24M01_HIGH_START) {
    addr -= M24M01_HIGH_START;
    adev = &dev->i2c_h;
    dev->oplen = len;
  } else {
    adev = &dev->i2c_l;
    dev->oplen = MIN(M24M01_HIGH_START - addr, len);
  }

  // 256 byte page boundary
  dev->oplen = MIN(dev->oplen, 256 - (addr & 0xff));

  dev->tmp[0] = addr >> 8;
  dev->tmp[1] = addr & 0xff;
  memcpy(&dev->tmp[2], buf, dev->oplen);

  dev->seq[0].dir = I2C_DEV_TX;
  dev->seq[0].buf = &dev->tmp[0];
  dev->seq[0].len = 2 + dev->oplen;
  dev->seq[0].gen_stop = I2C_DEV_STOP;

  EEDBG("write, sequence addr:%08x len:%i\n", addr, dev->oplen);
  res = I2C_DEV_sequence(adev, &dev->seq[0], 1);

  return res;
}

int m24m01_write(m24m01_dev *dev, u32_t addr, u8_t *buf, u32_t len) {
  if (addr > M24M01_SIZE || (addr+len) > M24M01_SIZE) {
    return I2C_ERR_M24M01_ADDRESS_RANGE_BAD;
  }
  if (dev->state != M24M01_IDLE) {
    return I2C_ERR_DEV_BUSY;
  }
  dev->addr = addr;
  dev->data = buf;
  dev->len = len;
  dev->oplen = 0;
  dev->state = M24M01_WRITE;
  dev->query_tries = 0;

  int res = _m24m01_write(dev, addr, buf, len);

  if (res < I2C_OK) {
    dev->state = M24M01_IDLE;
  }

  return res;
}
/////////////////////////////////////////////////////////////////////////// CLI

#ifndef CONFIG_CLI_M24M01_OFF
#include "cli.h"
#include "miniutils.h"

static m24m01_dev m24_dev;
static u8_t cli_buf[16];
static u32_t cli_addr;
static u32_t cli_len;
static u32_t cli_oplen;
static bool r_w;

static void cli_m24_cb(m24m01_dev *dev, int res) {
  if (res >= 0) {
    if (r_w) {
      // read
      int i;
      print("0x%08x: ", cli_addr);
      for (i = 0; i < cli_oplen; i++) {
        print("%02x ", cli_buf[i]);
      }
      print("\n");

      cli_len -= cli_oplen;
      cli_addr += cli_oplen;
      if (cli_len > 0) {
        cli_oplen = MIN(sizeof(cli_buf), cli_len);
        res = m24m01_read(&m24_dev, cli_addr, cli_buf, cli_oplen);
      }
    } else {
      print("OK\n");
    }
  }
  if (res < 0) {
    print("m24m01 async err: %i\n", res);
  }
}

static s32_t cli_m24_open(u32_t argc, u8_t bus, bool e1, bool e2, u32_t speed) {
  if (argc == 3) {
    speed = 100000;
  } else if (argc != 4) {
    return CLI_ERR_PARAM;
  }

  m24m01_open(&m24_dev, _I2C_BUS(bus), e1, e2, speed, cli_m24_cb);

  return CLI_OK;
}

static s32_t cli_m24_close(u32_t argc) {
  m24m01_close(&m24_dev);
  return CLI_OK;
}

static s32_t cli_m24_rd(u32_t argc, u32_t addr, u32_t len) {
  if (argc != 2 || IS_STRING((void *)addr) || IS_STRING((void *)len)) return CLI_ERR_PARAM;
  cli_addr = addr;
  cli_len = len;
  cli_oplen = MIN(sizeof(cli_buf), cli_len);
  r_w = TRUE;
  int res = m24m01_read(&m24_dev, cli_addr, cli_buf, cli_oplen);
  return res;
}

static s32_t cli_m24_wr(u32_t argc, u32_t addr, ...) {
  if (argc < 2 || IS_STRING((void *)addr)) return CLI_ERR_PARAM;
  r_w = FALSE;

  va_list va;
  va_start(va, addr);
  u32_t len = 0;
  u32_t i;
  for (i = 0; i < argc - 1; i++) {
    int b = va_arg(va, int);
    cli_buf[len++] = (u8_t)b;
    if (len >= sizeof(cli_buf)) break;
  }
  va_end(va);

  int res = m24m01_write(&m24_dev, addr, cli_buf, len);

  return res;
}

CLI_MENU_START(m24m01)
CLI_FUNC("close", cli_m24_close, "Closes m24m01 device")
CLI_FUNC("open", cli_m24_open, "Opens m24m01 device\n"
        "open <bus> <e1> <e2> (<bus_speed>)\n"
        "ex: open 0 0 0 100000\n")
CLI_FUNC("rd", cli_m24_rd, "Reads m24m01 data\n"
        "rd <addr> <len>\n")
CLI_FUNC("wr", cli_m24_wr, "Writes to m24m01 device\n"
        "wr <addr> (<byte_data>)*\n"
        "ex: wr 0x1000 0x12 0x34 0x56\n")
CLI_MENU_END


#endif



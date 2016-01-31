/*
 * cli_bus.c
 *
 *  Created on: Jan 29, 2016
 *      Author: petera
 */

#include "cli.h"
#include "linker_symaccess.h"
#include <stdarg.h>
#include "miniutils.h"

#ifdef CONFIG_UART

#include "uart_driver.h"

static s32_t cli_uart_write(u32_t argc, int uart, char* data) {
  if (argc != 2 || !IS_STRING(data)) {
    return CLI_ERR_PARAM;
  }
  if (uart < 0 || uart >= CONFIG_UART_CNT) {
    return CLI_ERR_PARAM;
  }
  char c;
  while ((c = *data++) != 0) {
    UART_put_char(_UART(uart), c);
  }
  return CLI_OK;
}

static s32_t cli_uart_read(u32_t argc, int uart, int numchars) {
  if (argc < 1 || argc > 2) {
    return CLI_ERR_PARAM;
  }
  if (uart < 0 || uart >= CONFIG_UART_CNT) {
    return CLI_ERR_PARAM;
  }
  if (argc == 1) {
    numchars = 0x7fffffff;
  }
  int l = UART_rx_available(_UART(uart));
  l = MIN(l, numchars);
  int ix = 0;
  while (ix++ < l) {
    print("%c", UART_get_char(_UART(uart)));
  }
  print("\n%i bytes read\n", l);
  return CLI_OK;
}

static s32_t cli_uart_conf(u32_t argc, int uart, int speed) {
  if (argc != 2) {
    return CLI_ERR_PARAM;
  }
  if (IS_STRING((char *)uart) || IS_STRING((char *)speed) || uart < 0 || uart >= CONFIG_UART_CNT) {
    return CLI_ERR_PARAM;
  }
  UART_config(_UART(uart), speed,
      UART_DATABITS_8, UART_STOPBITS_1, UART_PARITY_NONE, UART_FLOWCONTROL_NONE, TRUE);

  return CLI_OK;
}


CLI_MENU_START(uart)
CLI_FUNC("wr", cli_uart_write, "Writes to uart\n"
        "wr <uart> <string>\n"
        "ex: wr 2 \"foo\"\n")
CLI_FUNC("rd", cli_uart_read, "Reads from uart\n"
        "rd <uart> (<numchars>)\n"
        "numchars - number of chars to read, if omitted uart is drained\n"
        "ex: rd 2 10\n")
CLI_FUNC("cfg", cli_uart_conf, "Configure uart\n"
        "cfg <uart> <speed>\n"
        "ex: cfg 2 9600\n")
CLI_MENU_END

#endif // CONFIG_UART



#if defined(CONFIG_I2C) && defined(CONFIG_I2C_DEVICE)
#include "i2c_driver.h"
#include "i2c_dev.h"

static u8_t i2c_dev_reg = 0;
static u8_t i2c_dev_val = 0;
static i2c_dev i2c_device;
static u8_t i2c_wr_data[2];
static u8_t i2c_scan_addr, i2c_cli_bus;
static i2c_dev_sequence i2c_r_seqs[] = {
    I2C_SEQ_TX(&i2c_dev_reg, 1),
    I2C_SEQ_RX_STOP(&i2c_dev_val, 1)
};
static i2c_dev_sequence i2c_w_seqs[] = {
    I2C_SEQ_TX_STOP(i2c_wr_data, 2) ,
};

static void i2c_test_cb(i2c_dev *dev, int result) {
  if (result >= 0) {
    print("I2C bus:%i addr:0x%02x reg:0x%02x val:0x%02x 0b%08b\n", i2c_cli_bus, dev->addr, i2c_dev_reg, i2c_dev_val, i2c_dev_val);
  } else {
    print("ERR: i2c error %i\n", result);
  }
  I2C_DEV_close(&i2c_device);
}

static int cli_i2c_read(u32_t argc, int bus, int addr, int reg) {
  if (argc != 3) {
    return CLI_ERR_PARAM;
  }
  if (bus < 0 || bus >= I2C_MAX_ID) {
    return CLI_ERR_PARAM;
  }

  i2c_cli_bus = bus;
  I2C_DEV_init(&i2c_device, 100000, _I2C_BUS(bus), addr);
  I2C_DEV_set_callback(&i2c_device, i2c_test_cb);
  I2C_DEV_open(&i2c_device);
  i2c_dev_reg = reg;
  int res = I2C_DEV_sequence(&i2c_device, i2c_r_seqs, 2);
  if (res != I2C_OK) print("err: %i\n", res);
  return CLI_OK;
}

static int cli_i2c_write(u32_t argc, int bus, int addr, int reg, int data) {
  if (argc != 4) {
    return CLI_ERR_PARAM;
  }
  if (bus < 0 || bus >= I2C_MAX_ID) {
    return CLI_ERR_PARAM;
  }
  i2c_cli_bus = bus;
  I2C_DEV_init(&i2c_device, 100000, _I2C_BUS(bus), addr);
  I2C_DEV_set_callback(&i2c_device, i2c_test_cb);
  I2C_DEV_open(&i2c_device);
  i2c_wr_data[0] = reg;
  i2c_wr_data[1] = data;
  i2c_dev_reg = reg;
  i2c_dev_val = data;
  int res = I2C_DEV_sequence(&i2c_device, i2c_w_seqs, 1);
  if (res != I2C_OK) print("err: %i\n", res);
  return CLI_OK;
}

#define INVALID_ADDRESS(x) \
    (x & 0xf0) == 0b00000000 || \
    (x & 0xf0) == 0b11110000


void i2c_scan_report_task(u32_t addr, void *res) {
  if (addr == 0) {
    print("\n    0  2  4  6  8  a  c  e");
  }
  if ((addr & 0x0f) == 0) {
    print("\n%02x ", addr & 0xf0);
  }

  if (INVALID_ADDRESS(addr)) res = "-- ";

  print("%s", (char *) res);

  if (i2c_scan_addr < 0xff-2) {
    i2c_scan_addr += 2;
    if (INVALID_ADDRESS(i2c_scan_addr)) {
      task *report_scan_task = TASK_create(i2c_scan_report_task, 0);
      TASK_run(report_scan_task, i2c_scan_addr, NULL);
    } else {
      I2C_query(_I2C_BUS(i2c_cli_bus), i2c_scan_addr);
    }
  } else {
    print("\n");
  }
}

static void i2c_scan_cb_irq(i2c_bus *bus, int res) {
  task *report_scan_task = TASK_create(i2c_scan_report_task, 0);
  TASK_run(report_scan_task, bus->addr & 0xfe, res == I2C_OK ? "UP " : ".. ");
}

static int cli_i2c_scan(u32_t argc, int bus, int speed) {
  if (argc < 1 || argc > 2) {
    return CLI_ERR_PARAM;
  }
  if (IS_STRING((char *)bus) || (argc = 2 && IS_STRING((char *)speed)) || bus < 0 || bus >= I2C_MAX_ID) {
    return CLI_ERR_PARAM;
  }

  i2c_scan_addr = 0;
  i2c_cli_bus = bus;
  if (argc < 2) speed = 100000;
  int res = I2C_config(_I2C_BUS(bus), speed);
  if (res != I2C_OK) print("i2c config err %i\n", res);
  res = I2C_set_callback(_I2C_BUS(bus), i2c_scan_cb_irq);
  if (res != I2C_OK) print("i2c cb err %i\n", res);

  task *report_scan_task = TASK_create(i2c_scan_report_task, 0);
  TASK_run(report_scan_task, 0, NULL);

  return CLI_OK;
}

static int cli_i2c_reset(u32_t argc, int bus) {
  if (argc != 1) {
    return CLI_ERR_PARAM;
  }
  if (IS_STRING((char *)bus) || bus < 0 || bus >= I2C_MAX_ID) {
    return CLI_ERR_PARAM;
  }
  i2c_cli_bus = bus;
  I2C_reset(_I2C_BUS(bus));
  return CLI_OK;
}


CLI_MENU_START(i2c)
CLI_FUNC("rd", cli_i2c_read, "Reads one byte register from i2c\n"
        "rd <bus> <addr> <register>\n"
        "ex: rd 0 0x41 0x10\n")
CLI_FUNC("wr", cli_i2c_write, "Writes one byte register to i2c\n"
        "wr <bus> <addr> <register> <data>\n"
        "ex: wr 0 0x41 0x10 0x88\n")
CLI_FUNC("scan", cli_i2c_scan, "Scans i2c bus for devices\n"
        "scan <bus> (<clk_speed>)\n"
        "ex: scan 0\n")
CLI_FUNC("reset", cli_i2c_reset, "Resets i2c bus\n"
        "reset <bus>\n"
        "ex: reset 0\n")
CLI_MENU_END

#endif // CONFIG_I2C


// main bus menu

CLI_MENU_START(bus)
#ifdef CONFIG_UART
CLI_SUBMENU(uart, "uart", "UART submenu")
#endif
#if defined(CONFIG_I2C) && defined(CONFIG_I2C_DEVICE)
CLI_SUBMENU(i2c, "i2c", "I2C submenu")
#endif
CLI_MENU_END

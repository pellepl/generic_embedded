/*
 * cli_gpio.c
 *
 *  Created on: Jan 29, 2016
 *      Author: petera
 */

#include "cli.h"
#include "miniutils.h"
#include "gpio.h"

static s32_t _check_input(u32_t argc, u32_t *a_port, u32_t *a_pin) {
  if (argc != 2) return CLI_ERR_PARAM;

  u32_t port = *a_port;
  u32_t pin = *a_pin;

  if (IS_STRING((char *)pin)) return CLI_ERR_PARAM;

  if (IS_STRING((char *)port)) {
    char port_c = ((char *)port)[0];
    if (port_c >= 'a' && port_c <= 'z')
      *a_port = port_c - 'a';
    else if (port_c >= 'A' && port_c <= 'Z')
      *a_port = port_c - 'A';
  }

  if (*a_port < 0 || *a_port >= _IO_PORTS || *a_pin < 0 || *a_pin >= _IO_PINS) return CLI_ERR_PARAM;

  return CLI_OK;
}

static int cli_gpio_enable(u32_t argc, u32_t port, u32_t pin) {
  int res = _check_input(argc, &port, &pin);
  if (res == CLI_OK) gpio_enable(port, pin);
  return res;
}

static int cli_gpio_disable(u32_t argc, u32_t port, u32_t pin) {
  int res = _check_input(argc, &port, &pin);
  if (res == CLI_OK) gpio_disable(port, pin);
  return res;
}

static int cli_gpio_rd(u32_t argc, u32_t port, u32_t pin) {
  int res = _check_input(argc, &port, &pin);
  if (res == CLI_OK) {
    u32_t val = gpio_get(port, pin);
    print("%i\n", val);
  }
  return res;
}

CLI_MENU_START(gpio)
CLI_FUNC("hi", cli_gpio_enable, "Set pin high\n"
    "hi <port> <pin>\n")
CLI_FUNC("lo", cli_gpio_disable, "Set pin low\n"
    "lo <port> <pin>\n")
CLI_FUNC("rd", cli_gpio_rd, "Read pin status\n"
    "rd <port> <pin>\n")
CLI_MENU_END

/*
 * gpio.h
 *
 *  Created on: Sep 9, 2013
 *      Author: petera
 */

#ifndef GPIO_H_
#define GPIO_H_

#include "system.h"

typedef enum {
  PORTA = 0,
  PORTB,
  PORTC,
  PORTD,
  PORTE,
  PORTF,
  PORTG,
#if defined(PROC_STM32F4) || defined(PROC_STM32F7)
  PORTH,
  PORTI,
#endif
#if defined(PROC_STM32F7)
  PORTJ,
  PORTK,
#endif
  _IO_PORTS
} gpio_port;

typedef enum {
  PIN0 = 0,
  PIN1,
  PIN2,
  PIN3,
  PIN4,
  PIN5,
  PIN6,
  PIN7,
  PIN8,
  PIN9,
  PIN10,
  PIN11,
  PIN12,
  PIN13,
  PIN14,
  PIN15,
  _IO_PINS
} gpio_pin;


typedef enum {
  AF0 = 0,
  AF1,
  AF2,
  AF3,
  AF4,
  AF5,
  AF6,
  AF7,
  AF8,
  AF9,
  AF10,
  AF11,
  AF12,
  AF13,
  AF14,
  AF15,
  _AFS
} gpio_af;


typedef enum {
#ifdef PROC_STM32F7
  GPIO_CLK_LOW = GPIO_SPEED_FREQ_LOW,
  GPIO_CLK_MEDIUM = GPIO_SPEED_FREQ_MEDIUM,
  GPIO_CLK_HIGH = GPIO_SPEED_FREQ_HIGH,
  GPIO_CLK_VHIGH = GPIO_SPEED_VERY_HIGH,
#else
  CLK_2MHZ = 0,
#ifdef PROC_STM32F1
  CLK_10MHZ = 0,
#endif
#ifdef PROC_STM32F4
  CLK_25MHZ,
#endif
  CLK_50MHZ,
#ifdef PROC_STM32F4
  CLK_100MHZ,
#endif
#endif
  _IO_SPEEDS
} io_speed;


typedef enum {
  IN = 0,
  OUT,
  AF,
  ANALOG,
  _IO_MODES
} gpio_mode;


typedef enum {
  PUSHPULL = 0,
  OPENDRAIN,
  _IO_OUTTYPES
} gpio_outtype;

typedef enum {
  NOPULL= 0,
  PULLUP,
  PULLDOWN,
  _IO_PULLS
} gpio_pull;

typedef enum {
  FLANK_UP = 0,
  FLANK_DOWN,
  FLANK_BOTH,
  _IO_FLANKS
} gpio_flank;

typedef void (*gpio_interrupt_fn)(gpio_pin pin);

void gpio_init(void);

void *gpio_get_hw_port(gpio_port port);
int gpio_get_hw_pin(gpio_pin pin);

void gpio_config(gpio_port port, gpio_pin pin, io_speed speed, gpio_mode mode, gpio_af af, gpio_outtype outtype, gpio_pull pull);
void gpio_config_out(gpio_port port, gpio_pin pin, io_speed speed, gpio_outtype outtype, gpio_pull pull);
void gpio_config_in(gpio_port port, gpio_pin pin, io_speed speed);
void gpio_config_analog(gpio_port port, gpio_pin pin);
void gpio_config_release(gpio_port port, gpio_pin pin);

void gpio_enable(gpio_port port, gpio_pin pin);
void gpio_disable(gpio_port port, gpio_pin pin);
void gpio_set(gpio_port port, gpio_pin enable_pin, gpio_pin disable_pin);
u32_t gpio_get(gpio_port port, gpio_pin pin);

s32_t gpio_interrupt_config(gpio_port port, gpio_pin pin, gpio_interrupt_fn fn, gpio_flank flank);
void gpio_interrupt_deconfig(gpio_port port, gpio_pin pin);
void gpio_interrupt_mask_enable(gpio_port port, gpio_pin pin, bool clear_pending);
void gpio_interrupt_mask_disable(gpio_port port, gpio_pin pin);


#endif /* GPIO_H_ */

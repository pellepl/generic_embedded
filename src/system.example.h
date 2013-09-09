/*
 * system.h
 *
 *  Created on: Sep 7, 2013
 *      Author: petera
 */

#ifndef SYSTEM_H_
#define SYSTEM_H_

#include "stm32f4xx.h"
#include "types.h"

#define memcpy(d,s,n) __builtin_memcpy((d),(s),(n))
#define memset(s,t,n) __builtin_memset((s),(t),(n))
#define offsetof(st, m) \
     ((u32_t) ( (char *)&((st *)0)->m - (char *)0 ))

typedef GPIO_TypeDef * hw_io_port;
typedef uint16_t hw_io_pin;

#define GPIO_enable(port, pins) do { \
  (port)->BSRR = (pins); \
} while (0);
#define GPIO_disable(port, pins)  do { \
  (port)->BSRR = ((pins)<<16); \
} while (0);
#define GPIO_set(port, pin_ena, pin_dis)  do { \
  (port)->BSRR = (pin_ena) | ((pin_dis)<<16); \
} while (0);
#define GPIO_read(port, pin)  do { \
  (((port)->IDR & (pin)) != 0); \
} while (0);

// TODO
#define CONFIG_UART_CNT 0
#define STDOUT          0


#endif /* SYSTEM_H_ */

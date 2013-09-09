/*
 * uart.h
 *
 *  Created on: Jun 21, 2012
 *      Author: petera
 */

#ifndef UART_H_
#define UART_H_

#include "system.h"
#include "taskq.h"

#define UART_RX_BUFFER          512
#define UART_TX_BUFFER          512

#define UART_ALWAYS_SYNC_TX     0
#define UART_USE_STM_LIB        0

#ifndef UART_ALWAYS_SYNC_TX
#define UART_ALWAYS_SYNC_TX     0
#endif
#ifndef UART_USE_STM_LIB
#define UART_USE_STM_LIB        0
#endif

typedef void(*uart_rx_callback)(void *arg, u8_t c);

typedef struct {
  void* hw;
  struct {
    u8_t buf[UART_RX_BUFFER];
    u16_t wix;
    u16_t rix;
  } rx;
#if !UART_SYNC_TX
  struct {
    u8_t buf[UART_TX_BUFFER];
    volatile u16_t wix;
    u16_t rix;
  } tx;
#endif
  uart_rx_callback rx_f;
  void* arg;
  bool assure_tx;
  bool sync_tx;
} uart;

extern uart __uart_vec[CONFIG_UART_CNT];

#define _UART(x) (&__uart_vec[(x)])

#if UART_MAX_ID > 0
#define _U_STD (_UART(0))
#endif
#if UART_MAX_ID > 1
#define _U_DBG (_UART(1))
#endif
#if UART_MAX_ID > 2
#define _U_SPL (_UART(2))
#endif
#if UART_MAX_ID > 3
#define _U_BT  (_UART(3))
#endif

void UART_irq(uart *uart);
void UART_init();
void UART_assure_tx(uart *u, bool on);
bool UART_sync_tx(uart *u, bool on);
void UART_set_callback(uart *uart, uart_rx_callback rx_f, void* arg);
void UART_get_callback(uart *uart, uart_rx_callback *rx_f, void **arg);
s32_t UART_get_char(uart *uart);
s32_t UART_get_buf(uart *uart, u8_t* c, u16_t len);
s32_t UART_put_char(uart *uart, u8_t c);
void UART_tx_force_char(uart *uart, u8_t c);
s32_t UART_put_buf(uart *uart, u8_t* c, u16_t len);
u16_t UART_rx_available(uart *uart);
u16_t UART_tx_available(uart *uart);
void UART_tx_drain(uart *uart);
void UART_tx_flush(uart *uart);

#endif /* UART_H_ */

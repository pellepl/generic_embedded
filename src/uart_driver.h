/*
 * uart.h
 *
 *  Created on: Jun 21, 2012
 *      Author: petera
 */

#ifndef UART_H_
#define UART_H_

#include "system.h"

#ifndef UART_RX_BUFFER
#define UART_RX_BUFFER          512
#endif
#ifndef UART_TX_BUFFER
#define UART_TX_BUFFER          512
#endif

#ifndef UART_ALWAYS_SYNC_TX
#define UART_ALWAYS_SYNC_TX     0
#endif

typedef void(*uart_rx_callback)(void *arg, u8_t c);

typedef struct {
  void* hw;
  struct {
    u8_t buf[UART_RX_BUFFER];
    volatile u16_t wix;
    volatile u16_t rix;
  } rx;
#if !UART_SYNC_TX
  struct {
    u8_t buf[UART_TX_BUFFER];
    volatile u16_t wix;
    volatile u16_t rix;
  } tx;
#endif
  uart_rx_callback rx_f;
  void* arg;
  bool assure_tx;
  bool sync_tx;
} uart;

extern uart __uart_vec[CONFIG_UART_CNT];

#define _UART(x) (&__uart_vec[(x)])

typedef enum  {
  UART_CFG_DATABITS_8 = 0,
  UART_CFG_DATABITS_9 = 9,
} UART_databits;

typedef enum  {
  UART_CFG_STOPBITS_0_5 = 0,
  UART_CFG_STOPBITS_1,
  UART_CFG_STOPBITS_1_5,
  UART_CFG_STOPBITS_2,
} UART_stopbits;

typedef enum  {
  UART_CFG_PARITY_NONE = 0,
  UART_CFG_PARITY_EVEN,
  UART_CFG_PARITY_ODD,
} UART_parity;

typedef enum  {
  UART_CFG_FLOWCONTROL_NONE = 0,
  UART_CFG_FLOWCONTROL_RTS,
  UART_CFG_FLOWCONTROL_CTS,
  UART_CFG_FLOWCONTROL_RTS_CTS,
} UART_flowcontrol;

void UART_irq(uart *uart);
void UART_init();
bool UART_assure_tx(uart *u, bool on);
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
bool UART_config(uart *uart, u32_t baud, UART_databits databits,
    UART_stopbits stopbits, UART_parity parity, UART_flowcontrol flowcontrol,
    bool activate);

#endif /* UART_H_ */

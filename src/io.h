/*
 * io.h
 *
 *  Created on: Sep 10, 2013
 *      Author: petera
 */

#ifndef IO_H_
#define IO_H_

#include "system.h"

#ifdef CONFIG_UART
#include "uart_driver.h"
#endif

typedef enum {
  io_uart = 0,
#ifdef CONFIG_USB_VCD
  io_usb,
#endif
  io_file,
  io_memory,
  //io_memory_dma?,
  io_ringbuffer,
} io_media;

typedef void(*io_rx_cb)(u8_t io, void *arg, u16_t available);

#ifndef CONFIG_IO_MAX
#define CONFIG_IO_MAX 4
#endif

// If assure_tx is enabled io sends non-blocking if possible,
// otherwise blocks until all is sent.
// If assure_tx is disabled, overflowed data is discarded.
bool IO_assure_tx(u8_t io, bool on);
// If blocking_tx is on, io will always block until all is sent
bool IO_blocking_tx(u8_t io, bool on);
// Set rx callback, set NULL if no callback is wanted
void IO_set_callback(u8_t io, io_rx_cb cb, void *arg);
// Get rx callback
void IO_get_callback(u8_t io, io_rx_cb *cb, void **arg);
// returns a character from io
s32_t IO_get_char(u8_t io);
// returns a buffer from io
s32_t IO_get_buf(u8_t io, u8_t *buf, u16_t len);
// sends a character to io
s32_t IO_put_char(u8_t io, u8_t c);
// sends a character to io now, pronto, blocking, disregarding what is currently unsent
void IO_tx_force_char(u8_t io, u8_t c);
// discards current unsent data
void IO_tx_drain(u8_t io);
// flushes out unsent data blocking
void IO_tx_flush(u8_t io);
// sends a buffer to io
s32_t IO_put_buf(u8_t io, u8_t *buf, u16_t len);
// returns number of unread bytes from io rx buffer
s32_t IO_rx_available(u8_t io);
// returns number of free bytes in io tx buffer
s32_t IO_tx_available(u8_t io);

void IO_define(u8_t io, io_media media, u32_t id);

#endif /* IO_H_ */

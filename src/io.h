/*
 * io.h
 *
 *  Created on: Sep 10, 2013
 *      Author: petera
 */

#ifndef IO_H_
#define IO_H_

#include "system.h"
#ifdef CONFIG_IO

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
#ifdef CONFIG_IO_NOASSURE
#define IO_assure_tx(...) (void)0;
#else
bool IO_assure_tx(u8_t io, bool on);
#endif
// If blocking_tx is on, io will always block until all is sent
#ifdef CONFIG_IO_NOBLOCK
#define IO_blocking_tx(...) (void)0;
#else
bool IO_blocking_tx(u8_t io, bool on);
#endif
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
#ifdef CONFIG_IO_NODRAIN
#define IO_tx_drain(...)
#else
void IO_tx_drain(u8_t io);
#endif
// flushes out unsent data blocking
#ifdef CONFIG_IO_NOFLUSH
#define IO_tx_flush(...)
#else
void IO_tx_flush(u8_t io);
#endif
// sends a buffer to io
s32_t IO_put_buf(u8_t io, u8_t *buf, u16_t len);
// returns number of unread bytes from io rx buffer
s32_t IO_rx_available(u8_t io);
// returns number of free bytes in io tx buffer
s32_t IO_tx_available(u8_t io);

void IO_define(u8_t io, io_media media, u32_t id);

#else

#define IO_assure_tx(...) (void)0;
#define IO_blocking_tx(...) (void)0;
#define IO_tx_drain(...)
#define IO_tx_flush(...)

#endif // CONFIG_IO

#endif /* IO_H_ */

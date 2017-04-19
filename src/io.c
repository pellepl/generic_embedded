/*
 * io.c
 *
 *  Created on: Sep 10, 2013
 *      Author: petera
 */

#include "io.h"
#include "ringbuf.h"

#ifdef CONFIG_UART
#include "uart_driver.h"
#endif
#ifdef CONFIG_USB_VCD
#include "usb_serial.h"
#endif


typedef struct {
  io_media media;
  u32_t media_id;
  io_rx_cb cb;
  void *cb_arg;
} io_bus_def;

static io_bus_def io_bus[CONFIG_IO_MAX];

#ifdef CONFIG_UART
static void io_uart_cb(void *arg, u8_t c) {
  u8_t io = (u8_t)((u32_t)arg);
  if (io_bus[io].cb) {
    io_bus[io].cb(io, io_bus[io].cb_arg, UART_rx_available(_UART(io_bus[io].media_id)));
  }
}
#endif

#ifdef CONFIG_USB_VCD
static void io_usb_cb(u16_t available, void *arg) {
  u8_t io = (u8_t)((u32_t)arg);
  if (io_bus[io].cb) {
    io_bus[io].cb(io, io_bus[io].cb_arg, available);
  }
}
#endif

#ifndef CONFIG_IO_NOASSURE
bool IO_assure_tx(u8_t io, bool on) {
  switch (io_bus[io].media) {
  case io_uart:
    return UART_assure_tx(_UART(io_bus[io].media_id), on);
#ifdef CONFIG_USB_VCD
  case io_usb:
    return USB_SER_assure_tx(on);
#endif
  case io_ringbuffer:
  case io_memory:
  case io_file:
    return FALSE;
  }
  return FALSE;
}
#endif

#ifndef CONFIG_IO_NOBLOCK
bool IO_blocking_tx(u8_t io, bool on) {
  switch (io_bus[io].media) {
  case io_uart:
    return UART_sync_tx(_UART(io_bus[io].media_id), on);
#ifdef CONFIG_USB_VCD
  case io_usb:
    return USB_SER_assure_tx(on);
#endif
  case io_ringbuffer:
  case io_memory:
  case io_file:
    return FALSE;
  }
  return FALSE;
}
#endif

void IO_set_callback(u8_t io, io_rx_cb cb, void *arg) {
  if (cb) {
    io_bus[io].cb = cb;
    io_bus[io].cb_arg = arg;
  } else {
    io_bus[io].cb = NULL;
  }

  switch (io_bus[io].media) {
  case io_uart:
    UART_set_callback(_UART(io_bus[io].media_id), cb ? io_uart_cb : (void*)NULL, (void*)(u32_t)io);
    break;
#ifdef CONFIG_USB_VCD
  case io_usb:
    USB_SER_set_rx_callback(io_usb_cb, (void*)(u32_t)io);
    break;
#endif
  case io_ringbuffer:
  case io_memory:
  case io_file:
    break;
  }
}

void IO_get_callback(u8_t io, io_rx_cb *cb, void **arg) {
  *cb = io_bus[io].cb;
  *arg = io_bus[io].cb_arg;
}

s32_t IO_get_char(u8_t io) {
  u8_t c = -1;
  s32_t res;
  switch (io_bus[io].media) {
  case io_uart:
    return UART_get_char(_UART(io_bus[io].media_id));
#ifdef CONFIG_USB_VCD
  case io_usb:
    res = USB_SER_rx_char(&c);
    return res ? res : c;
#endif
  case io_file:
    return -1;
  case io_ringbuffer:
    res = ringbuf_getc((ringbuf *)io_bus[io].media_id, &c);
    return res ? res : c;
  case io_memory: {
    u8_t *p = (u8_t *)io_bus[io].media_id;
    c = *p++;
    io_bus[io].media_id = (u32_t)p;
    return c;
  }
  }

  return -1;
}

s32_t IO_get_buf(u8_t io, u8_t *buf, u16_t len) {
  switch (io_bus[io].media) {
  case io_uart:
    return UART_get_buf(_UART(io_bus[io].media_id), buf, len);
#ifdef CONFIG_USB_VCD
  case io_usb:
    return USB_SER_rx_buf(buf, len);
#endif
  case io_file:
    return -1;
  case io_ringbuffer:
    return ringbuf_get((ringbuf *)io_bus[io].media_id, buf, len);
  case io_memory: {
    u8_t *p = (u8_t *)io_bus[io].media_id;
    memcpy(buf, p, len);
    io_bus[io].media_id = (u32_t)p + len;
    return len;
  }
  }
  return -1;
}

s32_t IO_put_char(u8_t io, u8_t c) {
  switch (io_bus[io].media) {
  case io_uart:
    return UART_put_char(_UART(io_bus[io].media_id), c);
#ifdef CONFIG_USB_VCD
  case io_usb:
    return USB_SER_tx_char(c);
#endif
  case io_file:
    return -1;
  case io_ringbuffer:
    return ringbuf_putc((ringbuf *)io_bus[io].media_id, c);
  case io_memory: {
    u8_t *p = (u8_t *)io_bus[io].media_id;
    *p++ = c;
    io_bus[io].media_id = (u32_t)p;
    return c;
  }
  }
  return -1;
}

void IO_tx_force_char(u8_t io, u8_t c) {
  switch (io_bus[io].media) {
  case io_uart:
    UART_tx_force_char(_UART(io_bus[io].media_id), c);
    break;
#ifdef CONFIG_USB_VCD
  case io_usb:
    USB_SER_tx_char(c);
    break;
#endif
  case io_file:
    break;
  case io_ringbuffer:
    ringbuf_putc((ringbuf *)io_bus[io].media_id, c);
    break;
  case io_memory: {
    u8_t *p = (u8_t *)io_bus[io].media_id;
    *p++ = c;
    io_bus[io].media_id = (u32_t)p;
    break;
  }
  }
}

#ifndef CONFIG_IO_NODRAIN
void IO_tx_drain(u8_t io) {
  switch (io_bus[io].media) {
  case io_uart:
    UART_tx_drain(_UART(io_bus[io].media_id));
    break;
#ifdef CONFIG_USB_VCD
  case io_usb:
    USB_SER_tx_drain();
    break;
#endif
  case io_file:
    break;
  case io_ringbuffer:
    ringbuf_clear((ringbuf *)io_bus[io].media_id);
    break;
  case io_memory:
    break;
  }
}
#endif

#ifndef CONFIG_IO_NOFLUSH
void IO_tx_flush(u8_t io) {
  switch (io_bus[io].media) {
  case io_uart:
    UART_tx_flush(_UART(io_bus[io].media_id));
    break;
#ifdef CONFIG_USB_VCD
  case io_usb:
    USB_SER_tx_flush();
    break;
#endif
  case io_file:
  case io_ringbuffer:
  case io_memory:
    break;
  }
}
#endif

s32_t IO_put_buf(u8_t io, u8_t *buf, u16_t len) {
  switch (io_bus[io].media) {
  case io_uart:
    return UART_put_buf(_UART(io_bus[io].media_id), buf, len);
#ifdef CONFIG_USB_VCD
  case io_usb:
    return USB_SER_tx_buf(buf, len);
#endif
  case io_file:
    return -1;
  case io_ringbuffer:
    return ringbuf_put((ringbuf *)io_bus[io].media_id, buf, len);
  case io_memory: {
    u8_t *p = (u8_t *)io_bus[io].media_id;
    memcpy(p, buf, len);
    io_bus[io].media_id = (u32_t)p + len;
    return len;
  }
  }
  return -1;
}

s32_t IO_rx_available(u8_t io) {
  switch (io_bus[io].media) {
  case io_uart:
    return UART_rx_available(_UART(io_bus[io].media_id));
#ifdef CONFIG_USB_VCD
  case io_usb:
    return USB_SER_rx_avail();
#endif
  case io_file:
    return -1;
  case io_ringbuffer:
    return ringbuf_available((ringbuf *)io_bus[io].media_id);
  case io_memory:
    return sizeof(void *);
  }
  return -1;
}

s32_t IO_tx_available(u8_t io) {
  switch (io_bus[io].media) {
  case io_uart:
    return UART_tx_available(_UART(io_bus[io].media_id));
#ifdef CONFIG_USB_VCD
  case io_usb:
    return 8; // TODO PETER
#endif
  case io_file:
    return -1;
  case io_ringbuffer:
    return ringbuf_free((ringbuf *)io_bus[io].media_id);
  case io_memory:
    return sizeof(void *);
  }
  return -1;
}

void IO_define(u8_t io, io_media media, u32_t id) {
  io_bus[io].media = media;
  io_bus[io].media_id = id;
}

/*
 * io.c
 *
 *  Created on: Sep 10, 2013
 *      Author: petera
 */

#include "io.h"


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

bool IO_assure_tx(u8_t io, bool on) {
  switch (io_bus[io].media) {
  case io_uart:
    return UART_assure_tx(_UART(io_bus[io].media_id), on);
  case io_usb:
    return FALSE;
  case io_file:
    return FALSE;
  }
  return FALSE;
}

bool IO_blocking_tx(u8_t io, bool on) {
  switch (io_bus[io].media) {
  case io_uart:
    return UART_sync_tx(_UART(io_bus[io].media_id), on);
  case io_usb:
    return FALSE;
  case io_file:
    return FALSE;
  }
  return FALSE;
}

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
  case io_usb:
    usb_serial_set_rx_callback(io_usb_cb, arg);
    break;
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
  case io_usb:
    res = usb_serial_rx_char(&c);
    return res ? res : c;
  case io_file:
    return -1;
  }
  return -1;
}

s32_t IO_get_buf(u8_t io, u8_t *buf, u16_t len) {
  switch (io_bus[io].media) {
  case io_uart:
    return UART_get_buf(_UART(io_bus[io].media_id), buf, len);
  case io_usb:
    return usb_serial_rx_buf(buf, len);
  case io_file:
    return -1;
  }
  return -1;
}

s32_t IO_put_char(u8_t io, u8_t c) {
  switch (io_bus[io].media) {
  case io_uart:
    return UART_put_char(_UART(io_bus[io].media_id), c);
  case io_usb:
    return usb_serial_tx_char(c);
  case io_file:
    return -1;
  }
  return -1;
}

void IO_tx_force_char(u8_t io, u8_t c) {
  switch (io_bus[io].media) {
  case io_uart:
    UART_tx_force_char(_UART(io_bus[io].media_id), c);
  case io_usb:
    usb_serial_tx_char(c);
  case io_file:
    break;
  }
}

void IO_tx_drain(u8_t io) {
  switch (io_bus[io].media) {
  case io_uart:
    return UART_tx_drain(_UART(io_bus[io].media_id));
  case io_usb:
    break;
  case io_file:
    break;
  }
}

void IO_tx_flush(u8_t io) {
  switch (io_bus[io].media) {
  case io_uart:
    return UART_tx_flush(_UART(io_bus[io].media_id));
  case io_usb:
    break;
  case io_file:
    break;
  }
}

s32_t IO_put_buf(u8_t io, u8_t *buf, u16_t len) {
  switch (io_bus[io].media) {
  case io_uart:
    return UART_put_buf(_UART(io_bus[io].media_id), buf, len);
  case io_usb:
    return usb_serial_tx_buf(buf, len);
  case io_file:
    return -1;
  }
  return -1;
}

s32_t IO_rx_available(u8_t io) {
  switch (io_bus[io].media) {
  case io_uart:
    return UART_rx_available(_UART(io_bus[io].media_id));
  case io_usb:
    return usb_serial_rx_avail();
  case io_file:
    return -1;
  }
  return -1;
}

s32_t IO_tx_available(u8_t io) {
  switch (io_bus[io].media) {
  case io_uart:
    return UART_tx_available(_UART(io_bus[io].media_id));
  case io_usb:
    return 8; // TODO PETER
  case io_file:
    return -1;
  }
  return -1;
}

void IO_define(u8_t io, io_media media, u32_t id) {
  io_bus[io].media = media;
  io_bus[io].media_id = id;
}

/*
 * usb_serial.h
 *
 *  Created on: Sep 10, 2013
 *      Author: petera
 */

#ifndef USB_SERIAL_H_
#define USB_SERIAL_H_

#include "system.h"

typedef void(*usb_serial_rx_cb)(u16_t available, void *arg);

void usb_serial_init(void);
s32_t usb_serial_tx_char(u8_t c);
s32_t usb_serial_tx_buf(u8_t *buf, u16_t len);
s32_t usb_serial_rx_char(u8_t *c);
s32_t usb_serial_rx_buf(u8_t *buf, u16_t len);
u16_t usb_serial_rx_avail(void);
void usb_serial_set_rx_callback(usb_serial_rx_cb cb, void *arg);
void usb_serial_get_rx_callback(usb_serial_rx_cb *cb, void **arg);

#endif /* USB_SERIAL_H_ */

/*
 * usb_vcd.h
 *
 *  Created on: Sep 7, 2013
 *      Author: petera
 */

#ifndef USB_VCD_H_
#define USB_VCD_H_

void usb_init(void);
void usb_tx(char c);
void usb_tx_buf(u8_t *buf, u16_t len);

#endif /* USB_VCD_H_ */

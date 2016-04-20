/*
 * miniutils_config.h
 *
 *  Created on: Sep 7, 2013
 *      Author: petera
 */

#ifndef MINIUTILS_CONFIG_H_
#define MINIUTILS_CONFIG_H_

#include "system.h"
#include "uart_driver.h"
#include "usb_vcd.h"

#error this is but an example only


// enable formatted float printing support
#define MINIUTILS_PRINT_FLOAT
// enable formatted longlong printing support
#define MINIUTILS_PRINT_LONGLONG
// enabel base64 encoding/decoding
#define MINIUTILS_BASE64

#define PUTC(p, c)  \
  if ((int)(p) == STDOUT) \
    usb_tx((c)); \
  else if ((int)(p) < 256) \
    UART_put_char(_UART((int)(p-UART_PUTC_OFFSET)), (c)); \
  else \
    *((char*)(p)++) = (c);
#define PUTB(p, b, l)  \
  if ((int)(p) == STDOUT) \
    usb_tx_buf((b), (l)); \
  else if ((int)(p) < 256) \
    UART_put_buf(_UART((int)(p-UART_PUTC_OFFSET)), (u8_t*)(b), (int)(l)); \
  else { \
    int ____l = (l); \
    memcpy((char*)(p),(b),____l); \
    (p)+=____l; \
  }


#endif /* MINIUTILS_CONFIG_H_ */

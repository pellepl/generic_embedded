/*
 * usb_impl.c
 *
 *  Created on: Sep 3, 2013
 *      Author: petera
 */
#include "system.h"
#include "usb_vcd.h"

#include "usb_core.h"
#include "usbd_cdc_core.h"
#include "usbd_usr.h"
#include "usb_conf.h"
#include "usbd_desc.h"
#include "usb_dcd_int.h"
#include "usbd_cdc_core.h"

static USB_OTG_CORE_HANDLE USB_OTG_dev;
extern uint8_t  APP_Rx_Buffer []; /* Write CDC received data in this buffer.
                                     These data will be sent over USB IN endpoint
                                     in the CDC core functions. */
extern uint32_t APP_Rx_ptr_in;    /* Increment this pointer or roll it back to
                                     start address when writing received data
                                     in the buffer APP_Rx_Buffer. */

void usb_init(void)
{
  USBD_Init(&USB_OTG_dev, USB_OTG_FS_CORE_ID, &USR_desc, &USBD_CDC_cb, &USR_cb);
  USB_OTG_dev.cfg.low_power = 0;
}

void usb_tx(char c) {
  APP_Rx_Buffer[APP_Rx_ptr_in] = c;
  APP_Rx_ptr_in++;
  if (APP_Rx_ptr_in >= APP_RX_DATA_SIZE)
  {
    APP_Rx_ptr_in = 0;
  }
}

void usb_tx_buf(u8_t *buf, u16_t len) {
  if (APP_Rx_ptr_in + len >= APP_RX_DATA_SIZE) {
    u32_t avail = APP_RX_DATA_SIZE - APP_Rx_ptr_in;
    memcpy(&APP_Rx_Buffer[APP_Rx_ptr_in], buf, avail);
    buf += avail;
    len -= avail;
    APP_Rx_ptr_in = 0;
  }
  if (len > 0) {
    len = MIN(len, APP_RX_DATA_SIZE);
    memcpy(&APP_Rx_Buffer[APP_Rx_ptr_in], buf, len);
    APP_Rx_ptr_in += len;
    if (APP_Rx_ptr_in >= APP_RX_DATA_SIZE) {
      APP_Rx_ptr_in = 0;
    }
  }
}

void OTG_FS_WKUP_IRQHandler(void)
{
  if (USB_OTG_dev.cfg.low_power) {
//    *(uint32_t *) (0xE000ED10) &= 0xFFFFFFF9;
//    SystemInit();
//    USB_OTG_UngateClock(&USB_OTG_dev);
  }
  EXTI_ClearITPendingBit(EXTI_Line18);
}

void OTG_FS_IRQHandler(void)
{
  USBD_OTG_ISR_Handler(&USB_OTG_dev);
}

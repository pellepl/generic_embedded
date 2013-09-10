/*
 * usb_impl.c
 *
 *  Created on: Sep 3, 2013
 *      Author: petera
 */
#include "system.h"
#include "usb_vcd_impl.h"

#include "usb_core.h"
#include "usbd_cdc_core.h"
#include "usbd_usr.h"
#include "usb_conf.h"
#include "usbd_desc.h"
#include "usb_dcd_int.h"
#include "usbd_cdc_vcp.h"

#include "ringbuf.h"

static USB_OTG_CORE_HANDLE USB_OTG_dev;

extern uint8_t APP_Rx_Buffer[]; /* Write CDC received data in this buffer.
 These data will be sent over USB IN endpoint
 in the CDC core functions. */
extern uint32_t APP_Rx_ptr_in; /* Increment this pointer or roll it back to
 start address when writing received data
 in the buffer APP_Rx_Buffer. */


static u8_t rx_data[USB_VCD_RX_BUFFER];
static ringbuf rx_rb;
static usb_serial_rx_cb rx_cb = NULL;

static void *rx_cb_arg = 0;

static uint16_t VCP_Init(void);
static uint16_t VCP_DeInit(void);
static uint16_t VCP_Ctrl(uint32_t Cmd, uint8_t* Buf, uint32_t Len);
static uint16_t VCP_DataTx(uint8_t* Buf, uint32_t Len);
static uint16_t VCP_DataRx(uint8_t* Buf, uint32_t Len);

CDC_IF_Prop_TypeDef VCP_fops =
{ VCP_Init, VCP_DeInit, VCP_Ctrl, VCP_DataTx, VCP_DataRx };

void usb_serial_init(void) {
  USBD_Init(&USB_OTG_dev, USB_OTG_FS_CORE_ID, &USR_desc, &USBD_CDC_cb, &USR_cb);
  USB_OTG_dev.cfg.low_power = 0;
  rx_cb = 0;
  ringbuf_init(&rx_rb, rx_data, sizeof(rx_data));
}

void usb_serial_set_rx_callback(usb_serial_rx_cb cb, void *arg) {
  rx_cb = cb;
  rx_cb_arg = arg;
}

void usb_serial_get_rx_callback(usb_serial_rx_cb *cb, void **arg) {
  *cb = rx_cb;
  *arg = rx_cb_arg;
}

u16_t usb_serial_rx_avail(void) {
  return ringbuf_available(&rx_rb);
}

s32_t usb_serial_rx_char(u8_t *c) {
  return ringbuf_getc(&rx_rb, c);
}

s32_t usb_serial_rx_buf(u8_t *buf, u16_t len) {
  return ringbuf_get(&rx_rb, buf, len);
}


s32_t usb_serial_tx_char(u8_t c) {
  APP_Rx_Buffer[APP_Rx_ptr_in] = c;
  APP_Rx_ptr_in++;
  if (APP_Rx_ptr_in >= APP_RX_DATA_SIZE) {
    APP_Rx_ptr_in = 0;
  }
  return 1;
}

s32_t usb_serial_tx_buf(u8_t *buf, u16_t len) {
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
  return len;
}

void OTG_FS_WKUP_IRQHandler(void) {
  if (USB_OTG_dev.cfg.low_power) {
//    *(uint32_t *) (0xE000ED10) &= 0xFFFFFFF9;
//    SystemInit();
//    USB_OTG_UngateClock(&USB_OTG_dev);
  }
  EXTI_ClearITPendingBit(EXTI_Line18);
}

void OTG_FS_IRQHandler(void) {
  USBD_OTG_ISR_Handler(&USB_OTG_dev);
}

/* Private functions ---------------------------------------------------------*/
/**
 * @brief  VCP_Init
 *         Initializes the Media on the STM32
 * @param  None
 * @retval Result of the opeartion (USBD_OK in all cases)
 */
static uint16_t VCP_Init(void) {
  return USBD_OK;
}

/**
 * @brief  VCP_DeInit
 *         DeInitializes the Media on the STM32
 * @param  None
 * @retval Result of the opeartion (USBD_OK in all cases)
 */
static uint16_t VCP_DeInit(void) {
  return USBD_OK;
}

/**
 * @brief  VCP_Ctrl
 *         Manage the CDC class requests
 * @param  Cmd: Command code
 * @param  Buf: Buffer containing command data (request parameters)
 * @param  Len: Number of data to be sent (in bytes)
 * @retval Result of the opeartion (USBD_OK in all cases)
 */
static uint16_t VCP_Ctrl(uint32_t Cmd, uint8_t* Buf, uint32_t Len) {
  switch (Cmd) {
  case SEND_ENCAPSULATED_COMMAND:
    break;

  case GET_ENCAPSULATED_RESPONSE:
    break;

  case SET_COMM_FEATURE:
    break;

  case GET_COMM_FEATURE:
    break;

  case CLEAR_COMM_FEATURE:
    break;

  case SET_LINE_CODING:
    break;

  case GET_LINE_CODING:
    break;

  case SET_CONTROL_LINE_STATE:
    break;

  case SEND_BREAK:
    break;

  default:
    break;
  }

  return USBD_OK;
}

/**
 * @brief  VCP_DataTx
 *         CDC received data to be send over USB IN endpoint are managed in
 *         this function.
 * @param  Buf: Buffer of data to be sent
 * @param  Len: Number of data to be sent (in bytes)
 * @retval Result of the opeartion: USBD_OK if all operations are OK else VCP_FAIL
 */
static uint16_t VCP_DataTx(uint8_t* Buf, uint32_t Len) {
  //  TODO PETER stuff sent from uc gets here
  u32_t i;
  for (i = 0; i < Len; i++) {
    APP_Rx_Buffer[APP_Rx_ptr_in] = Buf[i];
    APP_Rx_ptr_in++;

    /* To avoid buffer overflow */
    if (APP_Rx_ptr_in == APP_RX_DATA_SIZE) {
      APP_Rx_ptr_in = 0;
    }
  }

  return USBD_OK;
}

/**
 * @brief  VCP_DataRx
 *         Data received over USB OUT endpoint are sent over CDC interface
 *         through this function.
 *
 *         @note
 *         This function will block any OUT packet reception on USB endpoint
 *         untill exiting this function. If you exit this function before transfer
 *         is complete on CDC interface (ie. using DMA controller) it will result
 *         in receiving more data while previous ones are still not sent.
 *
 * @param  Buf: Buffer of data to be received
 * @param  Len: Number of data received (in bytes)
 * @retval Result of the opeartion: USBD_OK if all operations are OK else VCP_FAIL
 */
static uint16_t VCP_DataRx(uint8_t* Buf, uint32_t Len) {
  ringbuf_put(&rx_rb, Buf, Len);
  if (rx_cb) {
    rx_cb( ringbuf_available(&rx_rb), rx_cb_arg );
  }

  return USBD_OK;
}


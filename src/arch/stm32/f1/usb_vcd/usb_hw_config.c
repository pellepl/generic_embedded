#include "system_config.h"
#include "usb_lib.h"
#include "usb_prop.h"
#include "usb_desc.h"
#include "usb_hw_config.h"
#include "usb_pwr.h"
#include "ringbuf.h"
#include "usb_serial.h"

ErrorStatus HSEStartUpStatus;
uint8_t tx_buf[USB_VCD_TX_BUF_SIZE];
uint8_t rx_buf[USB_VCD_RX_BUF_SIZE];
ringbuf tx_rb;
ringbuf rx_rb;

uint8_t USB_Tx_State = 0;
static void IntToUnicode(uint32_t value, uint8_t *pbuf, uint8_t len);

extern LINE_CODING linecoding;

void Enter_LowPowerMode(void) {
  /* Set the device state to suspend */
  bDeviceState = SUSPENDED;

  // TODO PETER TEST
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Pin = USB_DISCONNECT_PIN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
  GPIO_Init(USB_DISCONNECT, &GPIO_InitStructure);
  //NVIC_DisableIRQ(USB_LP_CAN1_RX0_IRQn);
}

void Leave_LowPowerMode(void) {
  DEVICE_INFO *pInfo = &Device_Info;

  /* Set the device state to the correct state */
  if (pInfo->Current_Configuration != 0) {
    /* Device configured */
    bDeviceState = CONFIGURED;
  } else {
    bDeviceState = ATTACHED;
  }
  /*Enable SystemCoreClock*/
  // TODO PETER TEST
  //NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn);
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Pin = USB_DISCONNECT_PIN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(USB_DISCONNECT, &GPIO_InitStructure);

  //SystemInit(); // TODO PETER - modification
}

void USB_Cable_Config(FunctionalState NewState) {
  if (NewState != DISABLE) {
    GPIO_ResetBits(USB_DISCONNECT, USB_DISCONNECT_PIN);
  } else {
    GPIO_SetBits(USB_DISCONNECT, USB_DISCONNECT_PIN);
  }
}

void USB_Receive(uint8_t* data_buffer, uint8_t Nb_bytes) {
  ringbuf_put(&rx_rb, data_buffer, Nb_bytes);
}

void Handle_USBAsynchXfer(void) {

  if (USB_Tx_State != 1) {
    u8_t *buf;
    int avail = ringbuf_available_linear(&tx_rb, &buf);
    if (avail == 0) {
      USB_Tx_State = 0;
      return;
    }

    if (avail > VIRTUAL_COM_PORT_DATA_SIZE) {
      avail = VIRTUAL_COM_PORT_DATA_SIZE;
    }

    USB_Tx_State = 1;
    UserToPMABufferCopy(buf, ENDP1_TXADDR, avail);
    ringbuf_get(&tx_rb, 0, avail);
    SetEPTxCount(ENDP1, avail);
    SetEPTxValid(ENDP1 );
  }

}

void Get_SerialNum(void) {
  uint32_t Device_Serial0, Device_Serial1, Device_Serial2;

  Device_Serial0 = *(uint32_t*) ID1;
  Device_Serial1 = *(uint32_t*) ID2;
  Device_Serial2 = *(uint32_t*) ID3;

  Device_Serial0 += Device_Serial2;

  if (Device_Serial0 != 0) {
    IntToUnicode(Device_Serial0, &Virtual_Com_Port_StringSerial[2], 8);
    IntToUnicode(Device_Serial1, &Virtual_Com_Port_StringSerial[18], 4);
  }
}

static void IntToUnicode(uint32_t value, uint8_t *pbuf, uint8_t len) {
  uint8_t idx = 0;

  for (idx = 0; idx < len; idx++) {
    if (((value >> 28)) < 0xA) {
      pbuf[2 * idx] = (value >> 28) + '0';
    } else {
      pbuf[2 * idx] = (value >> 28) + 'A' - 10;
    }

    value = value << 4;

    pbuf[2 * idx + 1] = 0;
  }
}

void usb_serial_init(void) {
  ringbuf_init(&tx_rb, tx_buf, sizeof(tx_buf));
  ringbuf_init(&rx_rb, rx_buf, sizeof(rx_buf));
  USB_Init();
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
  return ringbuf_putc(&tx_rb, c);
}

s32_t usb_serial_tx_buf(u8_t *buf, u16_t len) {
  return ringbuf_put(&tx_rb, buf, len);
}

void usb_serial_set_rx_callback(usb_serial_rx_cb cb, void *arg) {
  // TODO
}

void usb_serial_get_rx_callback(usb_serial_rx_cb *cb, void **arg) {
  // TODO
}

bool usb_serial_assure_tx(bool on) {
  // TODO
  return FALSE;
}

void usb_serial_tx_drain(void) {
  ringbuf_clear(&tx_rb);
}

void usb_serial_tx_flush(void) {
  // TODO
}


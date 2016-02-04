#ifndef __HW_CONFIG_H
#define __HW_CONFIG_H

#include "system.h"
#include "usb_type.h"

#define USB_VCD_TX_BUF_SIZE   2048
#define USB_VCD_RX_BUF_SIZE   512

void Enter_LowPowerMode(void);
void Leave_LowPowerMode(void);
void USB_Cable_Config (FunctionalState NewState);
void USB_Receive(uint8_t* data_buffer, uint8_t Nb_bytes);
void Handle_USBAsynchXfer (void);
void Get_SerialNum(void);

#endif  /*__HW_CONFIG_H*/

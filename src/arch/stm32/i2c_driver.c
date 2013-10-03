/*
 * i2c_driver.c
 *
 *  Created on: Jul 8, 2013
 *      Author: petera
 */

#include "i2c_driver.h"
#include "miniutils.h"

i2c_bus __i2c_bus_vec[I2C_MAX_ID];

#define I2C_HW(bus) ((I2C_TypeDef *)(bus)->hw)

int I2C_config(i2c_bus *bus, u32_t clock) {
  I2C_InitTypeDef I2C_InitStruct;
  I2C_StructInit(&I2C_InitStruct);
  I2C_Cmd(I2C_HW(bus), DISABLE);
  //I2C_DeInit(I2C_HW(bus));

  I2C_InitStruct.I2C_Mode = I2C_Mode_I2C;
  I2C_InitStruct.I2C_DutyCycle = I2C_DutyCycle_2;
  I2C_InitStruct.I2C_OwnAddress1 = 0x00; // never mind, will be master
  I2C_InitStruct.I2C_Ack = I2C_Ack_Enable;
  I2C_InitStruct.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
  I2C_InitStruct.I2C_ClockSpeed = clock;

  I2C_ITConfig(I2C_HW(bus), I2C_IT_ERR | I2C_IT_BUF | I2C_IT_EVT, ENABLE);
  
  I2C_Init(I2C_HW(bus), &I2C_InitStruct);

  I2C_AcknowledgeConfig(I2C_HW(bus), ENABLE);
  I2C_StretchClockCmd(I2C_HW(bus), ENABLE);

  return I2C_OK;
}

void I2C_init() {
  memset(__i2c_bus_vec, 0, sizeof(__i2c_bus_vec));
  _I2C_BUS(0)->hw = I2C1_PORT;
}

static void i2c_kickoff(i2c_bus *bus, bool set_ack, bool ack) {
  I2C_ITConfig(I2C_HW(bus), I2C_IT_ERR | I2C_IT_BUF | I2C_IT_EVT, ENABLE);
  I2C_Cmd(I2C_HW(bus), ENABLE);
  if (set_ack) {
    I2C_AcknowledgeConfig(I2C_HW(bus), ack ? ENABLE : DISABLE);
  }
  I2C_GenerateSTOP(I2C_HW(bus), DISABLE);
  I2C_GenerateSTART(I2C_HW(bus), ENABLE);
}

int I2C_rx(i2c_bus *bus, u8_t addr, u8_t *rx, u16_t len, bool gen_stop) {
  if (bus->state != I2C_S_IDLE) {
    return I2C_ERR_BUS_BUSY;
  }
  bus->state = I2C_S_GEN_START;
  bus->buf = rx;
  bus->len = len;
  bus->addr = addr | 0x01;
  bus->gen_stop = gen_stop;
  i2c_kickoff(bus, TRUE, len > 1);
  return I2C_OK;
}

int I2C_tx(i2c_bus *bus, u8_t addr, const u8_t *tx, u16_t len, bool gen_stop) {
  if (bus->state != I2C_S_IDLE) {
    return I2C_ERR_BUS_BUSY;
  }
  bus->state = I2C_S_GEN_START;
  bus->buf = (u8_t*) tx;
  bus->len = len;
  bus->addr = addr & 0xfe;
  bus->gen_stop = gen_stop;
  i2c_kickoff(bus, FALSE, FALSE);
  return I2C_OK;
}

int I2C_query(i2c_bus *bus, u8_t addr) {
  if (bus->state != I2C_S_IDLE) {
    return I2C_ERR_BUS_BUSY;
  }
  bus->state = I2C_S_GEN_START;
  bus->buf = 0;
  bus->len = 0;
  bus->addr = addr & 0xfe;
  bus->gen_stop = TRUE;
  i2c_kickoff(bus, FALSE, FALSE);
  return I2C_OK;
}

int I2C_set_callback(i2c_bus *bus,
    void (*i2c_bus_callback)(i2c_bus *bus, int res)) {
  if (bus->state != I2C_S_IDLE) {
    return I2C_ERR_BUS_BUSY;
  }
  bus->i2c_bus_callback = i2c_bus_callback;
  return I2C_OK;
}

int I2C_close(i2c_bus *bus) {
  I2C_ITConfig(I2C_HW(bus), I2C_IT_ERR | I2C_IT_BUF | I2C_IT_EVT, DISABLE);
  I2C_Cmd(I2C_HW(bus), DISABLE);
  bus->user_arg = 0;
  bus->user_p = 0;
  bus->bad_ev_counter = 0;
  if (bus->state != I2C_S_IDLE) {
    bus->state = I2C_S_IDLE;
    return I2C_ERR_BUS_BUSY;
  }
  return I2C_OK;
}

void I2C_register(i2c_bus *bus) {
  bus->attached_devices++;
}

void I2C_release(i2c_bus *bus) {
  bus->attached_devices--;
  if (bus->attached_devices == 0) {
    I2C_close(bus);
  }
}

bool I2C_is_busy(i2c_bus *bus) {
  return bus->state != I2C_S_IDLE;
}

static void i2c_finalize(i2c_bus *bus) {
  bus->state = I2C_S_IDLE;
  I2C_Cmd(I2C_HW(bus), DISABLE);
}

void I2C_reset(i2c_bus *bus) {
  I2C_SoftwareResetCmd(I2C_HW(bus), ENABLE);
  I2C_SoftwareResetCmd(I2C_HW(bus), DISABLE);
  i2c_finalize(bus);
}

static void i2c_error(i2c_bus *bus, int err, bool reset) {
  bus->bad_ev_counter = 0;
  if (bus->state != I2C_S_IDLE) {
    if (reset) {
      I2C_GenerateSTOP(I2C_HW(bus), ENABLE);
      I2C_ReceiveData(I2C_HW(bus) );
      I2C_reset(bus);
    }
    i2c_finalize(bus);
    I2C_ITConfig(I2C_HW(bus), I2C_IT_ERR | I2C_IT_BUF | I2C_IT_EVT, DISABLE);
    if (bus->i2c_bus_callback) {
      bus->i2c_bus_callback(bus, err);
    }
  }
}

u32_t I2C_phy_err(i2c_bus *bus) {
  return bus->phy_error;
}

void I2C_IRQ_err(i2c_bus *bus) {
  bool err = FALSE;
  bus->phy_error = 0;
  if (I2C_GetITStatus(I2C_HW(bus), I2C_IT_SMBALERT )) {
    DBG(D_I2C, D_WARN, "i2c_err: SMBus Alert\n");
    I2C_ClearITPendingBit(I2C_HW(bus), I2C_IT_SMBALERT );
    bus->phy_error |= 1 << I2C_ERR_PHY_SMBUS_ALERT;
    err = TRUE;
  }
  if (I2C_GetITStatus(I2C_HW(bus), I2C_IT_TIMEOUT )) {
    DBG(D_I2C, D_WARN, "i2c_err: Timeout or Tlow error\n");
    I2C_ClearITPendingBit(I2C_HW(bus), I2C_IT_TIMEOUT );
    bus->phy_error |= 1 << I2C_ERR_PHY_TIMEOUT;
    err = TRUE;
  }
  if (I2C_GetITStatus(I2C_HW(bus), I2C_IT_ARLO )) {
    DBG(D_I2C, D_WARN, "i2c_err: Arbitration lost\n");
    I2C_ClearITPendingBit(I2C_HW(bus), I2C_IT_ARLO );
    bus->phy_error |= 1 << I2C_ERR_PHY_ARBITRATION_LOST;
    err = TRUE;
  }
  if (I2C_GetITStatus(I2C_HW(bus), I2C_IT_PECERR )) {
    DBG(D_I2C, D_WARN, "i2c_err: PEC error\n");
    I2C_ClearITPendingBit(I2C_HW(bus), I2C_IT_PECERR );
    bus->phy_error |= 1 << I2C_ERR_PHY_PEC_ERR;
    err = TRUE;
  }
  if (I2C_GetITStatus(I2C_HW(bus), I2C_IT_OVR )) {
    DBG(D_I2C, D_WARN, "i2c_err: Overrun/Underrun flag\n");
    I2C_ClearITPendingBit(I2C_HW(bus), I2C_IT_OVR );
    bus->phy_error |= 1 << I2C_ERR_PHY_OVER_UNDERRUN;
    err = TRUE;
  }
  if (I2C_GetITStatus(I2C_HW(bus), I2C_IT_AF )) {
    DBG(D_I2C, D_WARN, "i2c_err: Acknowledge failure\n");
    I2C_ClearITPendingBit(I2C_HW(bus), I2C_IT_AF );
    bus->phy_error |= 1 << I2C_ERR_PHY_ACK_FAIL;
    err = TRUE;
  }
  if (I2C_GetITStatus(I2C_HW(bus), I2C_IT_BERR )) {
    DBG(D_I2C, D_WARN, "i2c_err: Bus error\n");
    I2C_ClearITPendingBit(I2C_HW(bus), I2C_IT_BERR );
    bus->phy_error |= 1 << I2C_ERR_PHY_BUS_ERR;
    err = TRUE;
  }

  if (err) {
    i2c_error(bus, I2C_ERR_PHY, FALSE);
  }
}

//#define I2C_HW_DEBUG(...) DBG(__VA_ARGS__)
#define I2C_HW_DEBUG(...)

void I2C_IRQ_ev(i2c_bus *bus) {
  u32_t ev = I2C_GetLastEvent(I2C_HW(bus) );
  I2C_HW_DEBUG(D_I2C, D_DEBUG, "i2c_ev: %08x\n", ev);
  switch (ev) {

  // EV5
  // from send start condition
  case I2C_EVENT_MASTER_MODE_SELECT :
    I2C_HW_DEBUG(D_I2C, D_DEBUG, "i2c_ev:   master mode\n");
    I2C_Send7bitAddress(I2C_HW(bus), bus->addr,
        (bus->addr & 0x01 ? I2C_Direction_Receiver : I2C_Direction_Transmitter));
    bus->bad_ev_counter = 0;
    break;

    // ------- TX --------

    // EV6
    // from send 7 bit address tx
  case I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED :
    I2C_HW_DEBUG(D_I2C, D_DEBUG, "i2c_ev:   master tx mode\n");
    if (bus->len > 0) {
      bus->state = I2C_S_TX;
      I2C_SendData(I2C_HW(bus), *bus->buf++);
      bus->len--;
    } else {
      // for address query
      I2C_GenerateSTOP(I2C_HW(bus), ENABLE);
      i2c_finalize(bus);
      if (bus->i2c_bus_callback) {
        bus->i2c_bus_callback(bus, I2C_OK);
      }
    }
    bus->bad_ev_counter = 0;
    break;
    // EV8
    // tx reg empty
  case I2C_EVENT_MASTER_BYTE_TRANSMITTING :
    I2C_HW_DEBUG(D_I2C, D_DEBUG, "i2c_ev:   master byte tx\n");
    if (bus->len > 0) {
      I2C_SendData(I2C_HW(bus), *bus->buf++);
      bus->len--;
    }
    bus->bad_ev_counter = 0;
    break;
    // EV8_2
    // tx reg transmitted, became empty
  case I2C_EVENT_MASTER_BYTE_TRANSMITTED :
    if (bus->state == I2C_S_TX) { // must check for tx here, at low speeds there might come spurios tx events
      I2C_HW_DEBUG(D_I2C, D_DEBUG, "i2c_ev:   master byte txed\n");
      if (bus->len == 0) {
        if (bus->gen_stop) {
          I2C_GenerateSTOP(I2C_HW(bus), ENABLE);
        }
        bool gen_cb = bus->state != I2C_S_IDLE;
        i2c_finalize(bus);
        if (bus->i2c_bus_callback && gen_cb) {
          bus->i2c_bus_callback(bus, I2C_TX_OK);
        }
      }
    }
    bus->bad_ev_counter = 0;
    break;

    // ------- RX --------

    // EV6
    // from send 7 bit address tx
  case I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED :
    I2C_HW_DEBUG(D_I2C, D_DEBUG, "i2c_ev:   master rx mode\n");
    bus->state = I2C_S_RX;
    if (bus->len <= 1) {
      if (bus->gen_stop) {
        I2C_GenerateSTOP(I2C_HW(bus), ENABLE);
      }
      I2C_AcknowledgeConfig(I2C_HW(bus), DISABLE);
    }
    bus->bad_ev_counter = 0;
    break;
    // EV7
    // rx reg filled
  case I2C_EVENT_MASTER_BYTE_RECEIVED :
    I2C_HW_DEBUG(D_I2C, D_DEBUG, "i2c_ev:   master byte rxed\n");
    u8_t data = I2C_ReceiveData(I2C_HW(bus) );
    *bus->buf++ = data;
    bus->len--;
    if (bus->len == 0) {
      if (bus->gen_stop) {
        I2C_GenerateSTOP(I2C_HW(bus), ENABLE);
      }
      I2C_AcknowledgeConfig(I2C_HW(bus), DISABLE);
      bool gen_cb = bus->state != I2C_S_IDLE;
      i2c_finalize(bus);
      if (bus->i2c_bus_callback && gen_cb) {
        bus->i2c_bus_callback(bus, I2C_RX_OK);
      }
    }
    bus->bad_ev_counter = 0;
    break;
  case 0x30000:
    // why oh why stm..?
    bus->bad_ev_counter = 0;
    break;

  default:
    // bad event
    DBG(D_I2C, D_WARN, "i2c_err: bad event %08x\n", ev);
    bus->bad_ev_counter++;
    if (bus->bad_ev_counter > 4) {
      i2c_error(bus, I2C_ERR_UNKNOWN_STATE, TRUE);
    }
    break;
  }
}

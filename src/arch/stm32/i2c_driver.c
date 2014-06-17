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

//#define I2C_HW_DBG(...) print("I2C:" ## __VA_ARGS__)
#define I2C_HW_DBG(...)

int I2C_config(i2c_bus *bus, u32_t clock) {
  I2C_InitTypeDef I2C_InitStruct;
  I2C_StructInit(&I2C_InitStruct);
  I2C_Cmd(I2C_HW(bus), DISABLE);
  I2C_DeInit(I2C_HW(bus));

  I2C_InitStruct.I2C_Mode = I2C_Mode_I2C;
  I2C_InitStruct.I2C_DutyCycle = I2C_DutyCycle_2;
  I2C_InitStruct.I2C_OwnAddress1 = 0x00; // never mind, will be master
  I2C_InitStruct.I2C_Ack = I2C_Ack_Enable;
  I2C_InitStruct.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
  I2C_InitStruct.I2C_ClockSpeed = clock;


  I2C_Init(I2C_HW(bus), &I2C_InitStruct);

  I2C_AcknowledgeConfig(I2C_HW(bus), ENABLE);
  I2C_Cmd(I2C_HW(bus), ENABLE); // todo 2
  I2C_StretchClockCmd(I2C_HW(bus), ENABLE);

  return I2C_OK;
}

void I2C_init() {
  memset(__i2c_bus_vec, 0, sizeof(__i2c_bus_vec));
  _I2C_BUS(0)->hw = I2C1_PORT;
}

static void i2c_kickoff(i2c_bus *bus, bool set_ack, bool ack) {
  I2C_ITConfig(I2C_HW(bus), I2C_IT_ERR | I2C_IT_BUF | I2C_IT_EVT, ENABLE);
  //I2C_Cmd(I2C_HW(bus), ENABLE); // todo 1
  if (set_ack) {
    I2C_AcknowledgeConfig(I2C_HW(bus), ack ? ENABLE : DISABLE);
  }
  I2C_GenerateSTOP(I2C_HW(bus), DISABLE);
  if (!bus->restart_generated) {
    I2C_LOG_START(bus);
    I2C_GenerateSTART(I2C_HW(bus), ENABLE);
  }
  bus->restart_generated = FALSE;
  bus->bad_ev_counter = 0;
}

int I2C_rx(i2c_bus *bus, u8_t addr, u8_t *rx, u16_t len, bool gen_stop) {
  if (bus->state != I2C_S_IDLE) {
    return I2C_ERR_BUS_BUSY;
  }
  I2C_HW_DBG("rx 0x%02x %i bytes STOP:%i\n", addr, len, gen_stop);
  bus->state = I2C_S_GEN_START;
  bus->op = I2C_OP_RX;
  bus->buf = rx;
  bus->len = len;
  bus->addr = addr | 0x01;
  bus->gen_stop = gen_stop;
  I2C_LOG_RESTART(bus);
  I2C_LOG_RX(bus, addr);
  i2c_kickoff(bus, TRUE, len > 1);
  return I2C_OK;
}

int I2C_tx(i2c_bus *bus, u8_t addr, const u8_t *tx, u16_t len, bool gen_stop) {
  if (bus->state != I2C_S_IDLE) {
    return I2C_ERR_BUS_BUSY;
  }
  I2C_HW_DBG("tx 0x%02x %i bytes STOP:%i\n", addr, len, gen_stop);
  bus->state = I2C_S_GEN_START;
  bus->op = I2C_OP_TX;
  bus->buf = (u8_t*) tx;
  bus->len = len;
  bus->addr = addr & 0xfe;
  bus->gen_stop = gen_stop;
  I2C_LOG_RESTART(bus);
  I2C_LOG_TX(bus, addr);
  i2c_kickoff(bus, FALSE, FALSE);
  return I2C_OK;
}

int I2C_query(i2c_bus *bus, u8_t addr) {
  if (bus->state != I2C_S_IDLE) {
    return I2C_ERR_BUS_BUSY;
  }
  bus->state = I2C_S_GEN_START;
  bus->op = I2C_OP_QUERY;
  bus->buf = 0;
  bus->len = 0;
  bus->addr = addr & 0xfe;
  bus->gen_stop = TRUE;
  I2C_LOG_RESTART(bus);
  I2C_LOG_QUERY(bus, addr);
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
  I2C_log_dump(bus);
  bus->state = I2C_S_IDLE;
  I2C_ITConfig(I2C_HW(bus), I2C_IT_ERR | I2C_IT_BUF | I2C_IT_EVT, DISABLE);
  //I2C_Cmd(I2C_HW(bus), DISABLE); // todo 1
}

void I2C_reset(i2c_bus *bus) {
  I2C_ITConfig(I2C_HW(bus), I2C_IT_ERR | I2C_IT_BUF | I2C_IT_EVT, DISABLE);
  // if an i2c device is queried but its ack is never clocked out, it
  // will pull SDA low forever. Clock out any pending acks and generate
  // STOP condition. Read twice if DR would be populated.
  I2C_LOG_STOP(bus);
  I2C_GenerateSTOP(I2C_HW(bus), ENABLE);
  I2C_ReceiveData(I2C_HW(bus));
  I2C_ReceiveData(I2C_HW(bus));
  //SYS_hardsleep_ms(1); // give some time to clock out
  volatile u32_t a = 0x10000;
  while (--a);

  I2C_SoftwareResetCmd(I2C_HW(bus), ENABLE);
  a = 0x10000;
  while (--a);
  I2C_SoftwareResetCmd(I2C_HW(bus), DISABLE);
  i2c_finalize(bus);
}

static void i2c_error(i2c_bus *bus, int err, bool reset) {
  bus->bad_ev_counter = 0;
  if (bus->state != I2C_S_IDLE) {
    if (reset) {
      I2C_reset(bus);
    }
    i2c_finalize(bus);
    if (bus->i2c_bus_callback) {
      I2C_LOG_CB(bus);
      bus->i2c_bus_callback(bus, err);
    }
  } else {
    if (reset) {
      I2C_reset(bus);
    }
  }
}

void I2C_log_dump(i2c_bus *bus) {
#ifdef I2C_EV_LOG_DBG
  u32_t i;
  print("====\n");
  for (i = 0; i < bus->log_ix; i++) {
    u32_t log = bus->log[i];
    if ((log & 0xffff0000) == 0x12340000) print(">>>RX  %02x\n", log & 0xff);
    else if ((log & 0xffff0000) == 0x12350000) print(">>>TX  %02x\n", log & 0xff);
    else if ((log & 0xffff0000) == 0x12360000) print(">>>QUE %02x\n", log & 0xff);
    else if (log == 0x01010000) print("START\n");
    else if (log == 0x01020000) print("STOP\n");
    else if ((log & 0xffff0000) == 0xffff0000) print("ERR %08b\n", log & 0xff);
    else if ((log & 0xff000000) == 0x80000000) print("EV  %06x\n", log & 0xffffff);
    else if ((log & 0xff000000) == 0xf0000000) print("UEV %06x\n", log & 0xffffff);
    else if ((log & 0xffff0000) == 0x43210000) print("CALLBACK\n");
    else print("?? %08x\n", log);
  }
  bus->log_ix = 0;
#endif
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
    I2C_LOG_ERR(bus, bus->phy_error);
    i2c_error(bus, I2C_ERR_PHY, FALSE);
  }
}


void I2C_IRQ_ev(i2c_bus *bus) {
  if (bus->state == I2C_S_IDLE) {
//    u32_t ev = I2C_GetLastEvent(I2C_HW(bus));
//    I2C_LOG_UNKNOWN_EV(bus, ev);
//    (void)ev;
//    i2c_error(bus, I2C_ERR_UNKNOWN_STATE, TRUE);
    return;
  }

  if (bus->op == I2C_OP_RX) {

    // ==== RX ====

    u32_t sr1 = I2C_HW(bus)->SR1;
    if ((sr1 & I2C_IT_SB) && bus->state == I2C_S_GEN_START) {
      // send address | 0x01 for rx
      I2C_HW_DBG("rx it sb\n");
      I2C_LOG_EV(bus, sr1);
      I2C_HW(bus)->DR = bus->addr | 0x01;
    } // if IT_SB
    else if ((sr1 & I2C_IT_ADDR) && bus->state == I2C_S_GEN_START) {
      // address sent and acknowledged
      // IT_ADDR bit cleared by reading SR1 & SR2
      (void)I2C_HW(bus)->SR2;
      I2C_LOG_EV(bus, sr1);
      bus->state = I2C_S_RX;
      if (bus->len == 2) {
        I2C_HW_DBG("rx it addr, len = 2\n");
        // RM0008 26.3.3 case len = 2
        I2C_ITConfig(I2C_HW(bus), I2C_IT_BUF, DISABLE); // kill RXNE interrupt, rely on BTF interrupt
        // POS=1 indicates that the ACK bit indicates ack or nack of next byte coming in the shift reg
        I2C_HW(bus)->CR1 |= I2C_CR1_POS;
        I2C_AcknowledgeConfig(I2C_HW(bus), DISABLE);
      }
      else if (bus->len <= 1) {
        // RM0008 26.3.3 case len = 1
        I2C_HW_DBG("rx it addr, len < 2\n");
        I2C_AcknowledgeConfig(I2C_HW(bus), DISABLE);
        if (bus->gen_stop) {
          I2C_LOG_STOP(bus);
          I2C_GenerateSTOP(I2C_HW(bus), ENABLE);
        } else {
          // the restart condition must be set before receiving last byte
          I2C_LOG_START(bus);
          I2C_GenerateSTART(I2C_HW(bus), ENABLE);
          bus->restart_generated = TRUE;
        }
      }
      else { // bus->len > 2
        // do naught, rely on RXNE interrupt
        I2C_HW_DBG("rx it addr, len > 2\n");
      }
    } // if IT_ADDR
    else if ((sr1 & I2C_IT_BTF) && bus->state == I2C_S_RX) {
      // data register full, shift register full, clock is now stretched
      I2C_LOG_EV(bus, sr1);
      if (bus->len == 2) {
        I2C_HW_DBG("rx it btf, len = 2\n");
        // these are the two last bytes to receive
        if (bus->gen_stop) {
          I2C_LOG_STOP(bus);
          I2C_GenerateSTOP(I2C_HW(bus), ENABLE);
        } else {
          // the restart condition must be set before receiving last byte
          I2C_LOG_START(bus);
          I2C_GenerateSTART(I2C_HW(bus), ENABLE);
          bus->restart_generated = TRUE;
        }
        *bus->buf++ = I2C_HW(bus)->DR;
        *bus->buf++ = I2C_HW(bus)->DR;
        bus->len -= 2;

        // clear POS
        I2C_HW(bus)->CR1 &= ~I2C_CR1_POS;

        i2c_finalize(bus);
        if (bus->i2c_bus_callback) {
          I2C_LOG_CB(bus);
          bus->i2c_bus_callback(bus, I2C_RX_OK);
        }
      }
      else if (bus->len == 3) {
        I2C_HW_DBG("rx it btf, len = 3\n");
        I2C_ITConfig(I2C_HW(bus), I2C_IT_BUF, DISABLE); // kill RXNE interrupts now, rely on BTF
        I2C_AcknowledgeConfig(I2C_HW(bus), DISABLE);
        // read N-2, await IT_BTF for the final 2 bytes
        *bus->buf++ = I2C_HW(bus)->DR;
        bus->len--;
      }
      else { // bus->len > 3
        I2C_HW_DBG("rx it btf, len > 3\n");
        *bus->buf++ = I2C_HW(bus)->DR;
        bus->len--;
      }
    } // if IT_BTF
    else if (sr1 & I2C_IT_RXNE) {
      I2C_LOG_EV(bus, sr1);
      ASSERT(bus->len != 2); // this should never happen as RXNE interrupts for 2 bytes are disabled
      if (bus->len == 3) {
        I2C_HW_DBG("rx it rxne, len = 3\n");
        I2C_ITConfig(I2C_HW(bus), I2C_IT_BUF, DISABLE); // kill RXNE interrupts now, rely on BTF
      }
      else {
        // bus->len > 3 || bus->len == 1
        I2C_HW_DBG("rx it rxne, len > 3\n");
        *bus->buf++ = I2C_HW(bus)->DR;
        bus->len--;
        if (bus->len == 0) {
          if (bus->gen_stop) {
            I2C_LOG_STOP(bus);
            I2C_GenerateSTOP(I2C_HW(bus), ENABLE);
          } else {
            // the restart condition must be set before receiving last byte
            I2C_LOG_START(bus);
            I2C_GenerateSTART(I2C_HW(bus), ENABLE);
            bus->restart_generated = TRUE;
          }
          bool gen_cb = bus->state == I2C_S_RX;
          i2c_finalize(bus);
          if (bus->i2c_bus_callback && gen_cb) {
            I2C_LOG_CB(bus);
            bus->i2c_bus_callback(bus, I2C_RX_OK);
          }
        }
      }
    } // if IT_RXNE
    else {
      // do not read SR2 here, this might clear IT_ADDR or IT_SB
      I2C_HW_DBG("rx it bad\n");
      I2C_LOG_UNKNOWN_EV(bus, sr1);
      bus->bad_ev_counter++;
    } // if any other interrupt

  } // if RX
  else {

    // ==== TX or QUERY ====

    u32_t sr1 = I2C_HW(bus)->SR1;

    I2C_LOG_EV(bus, sr1);
    if (sr1 & I2C_IT_SB) {
      // from send start condition
      I2C_HW_DBG("tx it sb\n");
      I2C_HW(bus)->DR = bus->addr & ~0x01;
    }
    else if (sr1 & I2C_IT_ADDR) {
      // from send 7 bit address tx
      // IT_ADDR bit cleared by reading SR1 & SR2
      (void)I2C_HW(bus)->SR2;
      if (bus->len > 0 && bus->op == I2C_OP_TX) {
        I2C_HW_DBG("tx it addr\n");
        bus->state = I2C_S_TX;
        I2C_HW(bus)->DR = *bus->buf++;
        bus->len--;
        if (bus->len == 0) {
          I2C_ITConfig(I2C_HW(bus), I2C_IT_BUF, DISABLE); // kill TXE interrupts now, rely on BTF
        }
      } else {
        // for address query
        I2C_HW_DBG("tx query it addr\n");
        I2C_LOG_STOP(bus);
        I2C_GenerateSTOP(I2C_HW(bus), ENABLE);
        i2c_finalize(bus);
        if (bus->i2c_bus_callback) {
          I2C_LOG_CB(bus);
          bus->i2c_bus_callback(bus, I2C_OK);
        }
      }
    }
    else if ((sr1 & I2C_IT_TXE) || (sr1 & I2C_IT_BTF)) {
      // transmitted or transmitting
      I2C_HW_DBG("tx it txed txing TXE:%i BTF:%i len:%i\n", (sr1 & I2C_IT_TXE) != 0, (sr1 & I2C_IT_BTF) != 0, bus->len);
      if (bus->len > 0) {
        I2C_HW(bus)->DR = *bus->buf++;
        bus->len--;
        if (bus->len == 0) {
          I2C_ITConfig(I2C_HW(bus), I2C_IT_BUF, DISABLE); // kill TXE interrupts now, rely on BTF
        }
      }
      else if (bus->state == I2C_S_TX && (sr1 & I2C_IT_BTF)) {
        // must check for tx here, at low speeds there might come spurios tx events
        if (bus->len == 0) {
          if (bus->gen_stop) {
            I2C_LOG_STOP(bus);
            I2C_GenerateSTOP(I2C_HW(bus), ENABLE);
          } else {
            I2C_LOG_START(bus);
            I2C_GenerateSTART(I2C_HW(bus), ENABLE);
            bus->restart_generated = TRUE;
          }
          bool gen_cb = bus->state == I2C_S_TX;
          i2c_finalize(bus);
          if (bus->i2c_bus_callback && gen_cb) {
            I2C_LOG_CB(bus);
            bus->i2c_bus_callback(bus, I2C_TX_OK);
          }
        }
      }
    }
    else {
      // bad event
      DBG(D_I2C, D_WARN, "i2c_err: bad event %02x\n", sr1);
      bus->bad_ev_counter ++;
      I2C_LOG_UNKNOWN_EV(bus, sr1);
    } // switch ev
  }
  if (bus->bad_ev_counter > 16) {
    i2c_error(bus, I2C_ERR_UNKNOWN_STATE, TRUE);
  }
}

/*
 * nrf905_driver.c
 *
 *  Created on: Nov 17, 2013
 *      Author: petera
 */

#include "nrf905_driver.h"

//#define DBG_STATE(s) print("NRF905DRV: state->%s\n", (s))
#define DBG_STATE(s)

static void nrf905_spi_dev_cb(spi_dev *d, int res) {
  nrf905 *nrf = (nrf905 *)d->user_data;
  ASSERT(nrf);

  nrf905_state st = nrf->state;
#ifndef CONFIG_SPI_KEEP_RUNNING
  SPI_DEV_close(nrf->spi_dev);
#endif

  if (res != SPI_OK) {
    NRF905_standby(nrf);
    if (nrf->cb) {
      nrf->cb(nrf, st, res);
    }
    return;
  }

  switch (st) {

  case NRF905_CONFIG:
  case NRF905_QUICK_CONFIG:
  case NRF905_CONFIG_TX_ADDR:
    NRF905_standby(nrf);
    if (nrf->cb) {
      nrf->cb(nrf, st, NRF905_OK);
    }
    break;

  case NRF905_READ_CONFIG: {
    NRF905_standby(nrf);
    // data in nrf->_buf[1..10], cfg struct in nrf->_scratch
    nrf905_config *cfg = (nrf905_config *)nrf->_scratch;
    u8_t *b = &nrf->_buf[0];

    cfg->channel_freq =            (b[1] | ((b[2] & 1)<<8));
    cfg->hfreq_pll =               (b[2] & 0b00000010) >> 1;
    cfg->pa_pwr =                  (b[2] & 0b00001100) >> 2;
    cfg->rx_reduced_power =        (b[2] & 0b00010000) >> 4;
    cfg->auto_retransmit =         (b[2] & 0b00100000) >> 5;
    cfg->tx_address_field_width =  (b[3] & 0xf0) >> 4;
    cfg->rx_address_field_width =  (b[3] & 0x0f);
    cfg->rx_payload_width =        b[4];
    cfg->tx_payload_width =        b[5];
    cfg->rx_address =              (b[6]<<24) | (b[7]<<16) | (b[8]<<8) | (b[9]);
    cfg->out_clk_freq =            (b[10] & 0b00000011);
    cfg->out_clk_enable =          (b[10] & 0b00000100) >> 2;
    cfg->crystal_frequency =       (b[10] & 0b00111000) >> 3;
    cfg->crc_en =                  (b[10] & 0b01000000) >> 6;
    cfg->crc_mode =                (b[10] & 0b10000000) >> 7;

    if (nrf->cb) {
      nrf->cb(nrf, st, NRF905_OK);
    }
    break;
  }
  case NRF905_TX_PRIME: {
    nrf->state = NRF905_TX;
    DBG_STATE("TX");
    // data_ready signal when packet sent
    GPIO_set(nrf->trx_ce_port, nrf->trx_ce_pin, 0);
    break;
  }
  case NRF905_TX_PRIME_CARRIER: {
    nrf->state = NRF905_TX_CARRIER;
    DBG_STATE("TX_CARRIER");
    // data_ready signal when packet sent, carrier continues
    GPIO_set(nrf->trx_ce_port, nrf->trx_ce_pin, 0);
    break;
  }
  case NRF905_RX_READ: {
    NRF905_standby(nrf);
    if (nrf->cb) {
      nrf->cb(nrf, st, NRF905_OK);
    }
    break;
  }
  default:
    NRF905_standby(nrf);
    if (nrf->cb) {
      nrf->cb(nrf, st, NRF905_ERR_ILLEGAL_STATE);
    }
    break;
  }
}


int NRF905_powerdown(nrf905 *nrf) {
  if (nrf->state != NRF905_STANDBY) {
    return NRF905_ERR_ILLEGAL_STATE;
  }
  GPIO_set(nrf->pwr_port, 0, nrf->pwr_pin);
  GPIO_set(nrf->trx_ce_port, 0, nrf->trx_ce_pin);
  GPIO_set(nrf->tx_en_port, 0, nrf->tx_en_pin);
  nrf->state = NRF905_POWERDOWN;
  DBG_STATE("POWDOW");

  return NRF905_OK;
}


int NRF905_standby(nrf905 *nrf) {
  GPIO_set(nrf->pwr_port, nrf->pwr_pin, 0);
  GPIO_set(nrf->trx_ce_port, 0, nrf->trx_ce_pin);
  GPIO_set(nrf->tx_en_port, 0, nrf->tx_en_pin);
  if (nrf->state == NRF905_POWERDOWN) {
    SYS_hardsleep_us(NRF905_STATE_TRANS_POWDOW_STDBY_US);
  }
  nrf->state = NRF905_STANDBY;
  DBG_STATE("STDBY");
  return NRF905_OK;
}


int NRF905_config(nrf905 *nrf, nrf905_config *cfg) {
  if (cfg->rx_payload_width == 0 || cfg->rx_payload_width > 32 ||
      cfg->tx_payload_width == 0 || cfg->tx_payload_width > 32) {
    return NRF905_ERR_BAD_CONFIG;
  }
  if (nrf->state != NRF905_STANDBY && nrf->state != NRF905_POWERDOWN) {
    return NRF905_ERR_ILLEGAL_STATE;
  }

  nrf->state = NRF905_CONFIG;
  DBG_STATE("CFG");

  memcpy(&nrf->config, cfg, sizeof(nrf905_config));
  u8_t *b = nrf->_buf;
  memset(b, 0, 11);

  b[0] = NRF905_SPI_W_CONFIG(0);

  b[1] =
      (cfg->channel_freq & 0xff);
  b[2] =
      (cfg->auto_retransmit << 5) |
      (cfg->rx_reduced_power << 4) |
      (cfg->pa_pwr << 2) |
      (cfg->hfreq_pll << 1) |
      ((cfg->channel_freq>>8) & 0x1);
  b[3] =
      (cfg->tx_address_field_width << 4) |
      (cfg->rx_address_field_width);
  b[4] =
      (cfg->rx_payload_width);
  b[5] =
      (cfg->tx_payload_width);
  b[6] =
      ((cfg->rx_address>>24) & 0xff);
  b[7] =
      ((cfg->rx_address>>16) & 0xff);
  b[8] =
      ((cfg->rx_address>>8) & 0xff);
  b[9] =
      (cfg->rx_address & 0xff);
  b[10] =
      (cfg->crc_mode << 7) |
      (cfg->crc_en << 6) |
      (cfg->crystal_frequency << 3) |
      (cfg->out_clk_enable << 2) |
      (cfg->out_clk_freq);

#ifndef CONFIG_SPI_KEEP_RUNNING
  SPI_DEV_open(nrf->spi_dev);
#endif
  int res = SPI_DEV_txrx(nrf->spi_dev, &b[0], 11, 0, 0);
  if (res != SPI_OK) {
    NRF905_standby(nrf);
#ifndef CONFIG_SPI_KEEP_RUNNING
    SPI_DEV_close(nrf->spi_dev);
#endif
  }

  return res;
}


int NRF905_quick_config_channel(nrf905 *nrf, u16_t channel_freq) {
  if (nrf->state != NRF905_STANDBY && nrf->state != NRF905_POWERDOWN) {
    return NRF905_ERR_ILLEGAL_STATE;
  }
  nrf->state = NRF905_QUICK_CONFIG;
  DBG_STATE("QCFG");
  u8_t *b = nrf->_buf;
  u16_t d = NRF905_SPI_CH_CONFIG(channel_freq, nrf->config.hfreq_pll, nrf->config.pa_pwr);
  b[0] = (d>>8) & 0xff;
  b[1] = d & 0xff;
#ifndef CONFIG_SPI_KEEP_RUNNING
  SPI_DEV_open(nrf->spi_dev);
#endif
  int res = SPI_DEV_txrx(nrf->spi_dev, &b[0], 2, 0, 0);
  if (res != SPI_OK) {
    NRF905_standby(nrf);
#ifndef CONFIG_SPI_KEEP_RUNNING
    SPI_DEV_close(nrf->spi_dev);
#endif
  }

  return res;
}


int NRF905_quick_config_pa(nrf905 *nrf, nrf905_cfg_pa_pwr pa_pwr) {
  if (nrf->state != NRF905_STANDBY && nrf->state != NRF905_POWERDOWN) {
    return NRF905_ERR_ILLEGAL_STATE;
  }
  nrf->state = NRF905_QUICK_CONFIG;
  DBG_STATE("QCFG");
  u8_t *b = nrf->_buf;

  u16_t d = NRF905_SPI_CH_CONFIG(nrf->config.channel_freq, nrf->config.hfreq_pll, pa_pwr);
  b[0] = (d>>8) & 0xff;
  b[1] = d & 0xff;
#ifndef CONFIG_SPI_KEEP_RUNNING
  SPI_DEV_open(nrf->spi_dev);
#endif
  int res = SPI_DEV_txrx(nrf->spi_dev, &b[0], 2, 0, 0);
  if (res != SPI_OK) {
    NRF905_standby(nrf);
#ifndef CONFIG_SPI_KEEP_RUNNING
    SPI_DEV_close(nrf->spi_dev);
#endif
  }

  return res;
}


int NRF905_read_config(nrf905 *nrf, nrf905_config *cfg) {
  if (nrf->state != NRF905_STANDBY && nrf->state != NRF905_POWERDOWN) {
    return NRF905_ERR_ILLEGAL_STATE;
  }

  u8_t *b = nrf->_buf;

  nrf->state = NRF905_READ_CONFIG;
  DBG_STATE("RCFG");
  memset(b, 0, 11);
  nrf->_scratch = cfg;

  b[0] = NRF905_SPI_R_CONFIG(0);

#ifndef CONFIG_SPI_KEEP_RUNNING
  SPI_DEV_open(nrf->spi_dev);
#endif
  int res = SPI_DEV_txrx(nrf->spi_dev, &b[0], 1, &b[1], 10);
  if (res != SPI_OK) {
    NRF905_standby(nrf);
#ifndef CONFIG_SPI_KEEP_RUNNING
    SPI_DEV_close(nrf->spi_dev);
#endif
  }

  return res;
}


int NRF905_config_tx_address(nrf905 *nrf, u8_t *addr) {
  if (nrf->state != NRF905_STANDBY && nrf->state != NRF905_POWERDOWN) {
    return NRF905_ERR_ILLEGAL_STATE;
  }

  nrf->state = NRF905_CONFIG_TX_ADDR;
  DBG_STATE("CFGTXADR");
  u8_t *b = nrf->_buf;

  b[0] = NRF905_SPI_W_TX_ADDRESS;
  u8_t addr_len;

  if (nrf->config.tx_address_field_width == NRF905_CFG_ADDRESS_FIELD_WIDTH_1) {
    addr_len = 1;
  } else if (nrf->config.tx_address_field_width == NRF905_CFG_ADDRESS_FIELD_WIDTH_4) {
    addr_len = 4;
  } else {
    addr_len = 0;
    ASSERT(FALSE);
  }
  memcpy(&b[1], addr, addr_len);
#ifndef CONFIG_SPI_KEEP_RUNNING
  SPI_DEV_open(nrf->spi_dev);
#endif
  int res = SPI_DEV_txrx(nrf->spi_dev, &b[0], 1+addr_len, 0, 0);
  if (res != SPI_OK) {
    NRF905_standby(nrf);
#ifndef CONFIG_SPI_KEEP_RUNNING
    SPI_DEV_close(nrf->spi_dev);
#endif
  }
  return res;
}


int NRF905_tx(nrf905 *nrf, u8_t *data, u8_t len) {
  if (nrf->state != NRF905_STANDBY) {
    return NRF905_ERR_ILLEGAL_STATE;
  }

  if (len > nrf->config.tx_payload_width) {
    return NRF905_ERR_BAD_CONFIG;
  }

  nrf->state = NRF905_TX_PRIME;
  DBG_STATE("TXPRIME");
  u8_t *b = nrf->_buf;

  GPIO_set(nrf->tx_en_port, nrf->tx_en_pin, 0);

  b[0] = NRF905_SPI_W_TX_PAYLOAD;

  memcpy(&b[1], data, len);
#ifndef CONFIG_SPI_KEEP_RUNNING
  SPI_DEV_open(nrf->spi_dev);
#endif
  int res = SPI_DEV_txrx(nrf->spi_dev, &b[0], 1+len, 0, 0);
  if (res != SPI_OK) {
    NRF905_standby(nrf);
#ifndef CONFIG_SPI_KEEP_RUNNING
    SPI_DEV_close(nrf->spi_dev);
#endif
  }
  return res;
}


int NRF905_rx(nrf905 *nrf) {
  if (nrf->state != NRF905_STANDBY) {
    return NRF905_ERR_ILLEGAL_STATE;
  }
  nrf->state = NRF905_RX_LISTEN;
  DBG_STATE("RX");

  GPIO_set(nrf->tx_en_port, 0, nrf->tx_en_pin);
  GPIO_set(nrf->trx_ce_port, nrf->trx_ce_pin, 0);

  return NRF905_OK;
}

int NRF905_tx_carrier(nrf905 *nrf) {
  if (nrf->state != NRF905_STANDBY) {
    return NRF905_ERR_ILLEGAL_STATE;
  }

  nrf->state = NRF905_TX_PRIME_CARRIER;
  DBG_STATE("TXPRIMECARRIER");
  u8_t *b = nrf->_buf;

  GPIO_set(nrf->tx_en_port, nrf->tx_en_pin, 0);

  b[0] = NRF905_SPI_W_TX_PAYLOAD;

#ifndef CONFIG_SPI_KEEP_RUNNING
  SPI_DEV_open(nrf->spi_dev);
#endif
  int res = SPI_DEV_txrx(nrf->spi_dev, &b[0], 2, 0, 0);
  if (res != SPI_OK) {
    NRF905_standby(nrf);
#ifndef CONFIG_SPI_KEEP_RUNNING
    SPI_DEV_close(nrf->spi_dev);
#endif
  }
  return res;
}


void NRF905_signal_data_ready(nrf905 *nrf) {
  switch (nrf->state) {
  case NRF905_TX: {
    NRF905_standby(nrf);
    if (nrf->cb) {
      nrf->cb(nrf, NRF905_TX, NRF905_OK);
    }
    break;
  }
  case NRF905_RX_LISTEN: {
    nrf->state = NRF905_RX_READ;
    DBG_STATE("READ");
    u8_t *b = nrf->_buf;
    GPIO_set(nrf->trx_ce_port, 0, nrf->trx_ce_pin);
    b[0] = NRF905_SPI_R_RX_PAYLOAD;
#ifndef CONFIG_SPI_KEEP_RUNNING
    SPI_DEV_open(nrf->spi_dev);
#endif
    int res = SPI_DEV_txrx(nrf->spi_dev, &b[0], 1, &b[1], nrf->config.rx_payload_width);
    if (res != SPI_OK) {
#ifndef CONFIG_SPI_KEEP_RUNNING
      SPI_DEV_close(nrf->spi_dev);
#endif
      NRF905_standby(nrf);
      if (nrf->cb) {
        nrf->cb(nrf, NRF905_RX_LISTEN, res);
      }
    }
    break;
  }
  default:
    break;
  }
}


void NRF905_init(nrf905 *nrf, spi_dev *spi_dev,
    nrf905_cb cb,
    hw_io_port pwr_port, hw_io_pin pwr_pin,
    hw_io_port trx_ce_port, hw_io_pin trx_ce_pin,
    hw_io_port tx_en_port, hw_io_pin tx_en_pin) {
  memset(nrf, 0, sizeof(nrf905));
  nrf->spi_dev = spi_dev;
  nrf->cb = cb;
  nrf->pwr_port = pwr_port;
  nrf->pwr_pin = pwr_pin;
  nrf->trx_ce_port = trx_ce_port;
  nrf->trx_ce_pin = trx_ce_pin;
  nrf->tx_en_port = tx_en_port;
  nrf->tx_en_pin = tx_en_pin;

  GPIO_set(pwr_port, 0, pwr_pin);
  GPIO_set(trx_ce_port, 0, trx_ce_pin);
  GPIO_set(tx_en_port, 0, tx_en_pin);

  nrf->state = NRF905_POWERDOWN;
  DBG_STATE("POWDOW");

  SPI_DEV_set_callback(spi_dev, nrf905_spi_dev_cb);
  SPI_DEV_set_user_data(spi_dev, nrf);
#ifdef CONFIG_SPI_KEEP_RUNNING
  SPI_DEV_open(spi_dev);
#endif

}

nrf905_state NRF905_get_state(nrf905 *nrf) {
  return nrf->state;
}

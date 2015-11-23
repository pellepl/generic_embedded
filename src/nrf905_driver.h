/*
 * nrf905_driver.h
 *
 *  Created on: Nov 17, 2013
 *      Author: petera
 */

#ifndef NRF905_DRIVER_H_
#define NRF905_DRIVER_H_

#include "system.h"
#include "spi_dev.h"

#define NRF905_OK                         0
#define NRF905_ERR_BUSY                   -4000
#define NRF905_ERR_BAD_CONFIG             -4001
#define NRF905_ERR_ILLEGAL_STATE          -4002

typedef enum {
  // power down
  NRF905_POWERDOWN = 0,
  // standby/ready-for-configuration
  NRF905_STANDBY,
  // configuration ongoing
  NRF905_CONFIG,
  // quick configuration ongoing
  NRF905_QUICK_CONFIG,
  // configuration readback ongoing
  NRF905_READ_CONFIG,
  // listens for packets, data-ready high when packet received
  NRF905_RX_LISTEN,
  // reading a receive packet
  NRF905_RX_READ,
  // setting tx address
  NRF905_CONFIG_TX_ADDR,
  // preparing for tx
  NRF905_TX_PRIME,
  // txing a packet, data-ready high when packet sent
  NRF905_TX,
  // preparing for tx carrier
  NRF905_TX_PRIME_CARRIER,
  // txing carrier
  NRF905_TX_CARRIER,
} nrf905_state;

typedef enum {
  NRF905_CFG_HFREQ_433 = 0,
  NRF905_CFG_HFREQ_868_915 = 1,
} nrf905_cfg_hfreq;

typedef enum {
  NRF905_CFG_PA_PWR_m10 = 0b00,
  NRF905_CFG_PA_PWR_m2 = 0b01,
  NRF905_CFG_PA_PWR_6 = 0b10,
  NRF905_CFG_PA_PWR_10 = 0b11
} nrf905_cfg_pa_pwr;

typedef enum {
  NRF905_CFG_RX_REDUCED_POWER_OFF = 0,
  NRF905_CFG_RX_REDUCED_POWER_ON,
} nrf905_cfg_rx_reduced_power;

typedef enum {
  NRF905_CFG_AUTO_RETRAN_OFF = 0,
  NRF905_CFG_AUTO_RETRAN_ON,
} nrf905_cfg_auto_retransmit;

typedef enum {
  NRF905_CFG_ADDRESS_FIELD_WIDTH_1 = 0b001,
  NRF905_CFG_ADDRESS_FIELD_WIDTH_4 = 0b100,
} nrf905_cfg_address_field_width;

typedef enum {
  NRF905_CFG_OUT_CLK_FREQ_4MHZ = 0b00,
  NRF905_CFG_OUT_CLK_FREQ_2MHZ = 0b01,
  NRF905_CFG_OUT_CLK_FREQ_1MHZ = 0b10,
  NRF905_CFG_OUT_CLK_FREQ_0_5MHZ = 0b11,
} nrf905_cfg_out_clk_freq;

typedef enum {
  NRF905_CFG_OUT_CLK_OFF = 0,
  NRF905_CFG_OUT_CLK_ON = 1
} nrf905_cfg_out_clk_enable;

typedef enum {
  NRF905_CFG_XTAL_FREQ_4MHZ = 0b000,
  NRF905_CFG_XTAL_FREQ_8MHZ = 0b001,
  NRF905_CFG_XTAL_FREQ_12MHZ = 0b010,
  NRF905_CFG_XTAL_FREQ_16MHZ = 0b011,
  NRF905_CFG_XTAL_FREQ_20MHZ = 0b100,
} nrf905_cfg_xtal_freq;

typedef enum {
  NRF905_CFG_CRC_OFF = 0,
  NRF905_CFG_CRC_ON = 1
} nrf905_cfg_crc;

typedef enum {
  NRF905_CFG_CRC_MODE_8BIT = 0,
  NRF905_CFG_CRC_MODE_16BIT = 1,
} nrf905_cfg_crc_mode;

typedef struct {
  u16_t channel_freq;
  nrf905_cfg_hfreq hfreq_pll;
  nrf905_cfg_pa_pwr pa_pwr;
  nrf905_cfg_rx_reduced_power rx_reduced_power;
  nrf905_cfg_auto_retransmit auto_retransmit;
  nrf905_cfg_address_field_width rx_address_field_width;
  nrf905_cfg_address_field_width tx_address_field_width;
  u8_t rx_payload_width;
  u8_t tx_payload_width;
  u32_t rx_address;
  nrf905_cfg_out_clk_freq out_clk_freq;
  nrf905_cfg_out_clk_enable out_clk_enable;
  nrf905_cfg_xtal_freq crystal_frequency;
  nrf905_cfg_crc crc_en;
  nrf905_cfg_crc_mode crc_mode;
} nrf905_config;

#define NRF905_STATE_TRANS_POWDOW_STDBY_US    30000
#define NRF905_STATE_TRANS_STDBY_RXTX_US      650
#define NRF905_STATE_TRANS_RXTX_TXRX_US       550

#define NRF905_SPI_W_CONFIG(r)          ( (0b0000 << 4) | ((r) & 0xf) )
#define NRF905_SPI_R_CONFIG(r)          ( (0b0001 << 4) | ((r) & 0xf) )
#define NRF905_SPI_W_TX_PAYLOAD         ( 0b00100000 )
#define NRF905_SPI_R_TX_PAYLOAD         ( 0b00100001 )
#define NRF905_SPI_W_TX_ADDRESS         ( 0b00100010 )
#define NRF905_SPI_R_TX_ADDRESS         ( 0b00100011 )
#define NRF905_SPI_R_RX_PAYLOAD         ( 0b00100100 )
#define NRF905_SPI_CH_CONFIG(ch,pll,pa) ( (0b1000<<12) | ((pa&0x3)<<10) | ((pll&1)<<9) | (ch&0x1ff) )

#define NRF905_RX_PKT_BUFFER(nrf) ( (u8_t*)(&(nrf)->_buf[1]) )
#define NRF905_RX_PKT_LEN(nrf) ( (nrf)->config.rx_payload_width )

typedef struct nrf905 nrf905;

/**
 * State/result callback function.
 * If state == NRF905_RX_READ and res == NRF905_OK a packet can be fetched using
 * macros NRF905_RX_PKT_BUFFER and NRF905_RX_PKT_LEN.
 */
typedef void (*nrf905_cb)(nrf905 *nrf, nrf905_state state, int res);

typedef struct nrf905 {
  nrf905_state state;
  nrf905_cb cb;
  nrf905_config config;
  spi_dev *spi_dev;
  u8_t _buf[34];
  void *_scratch;

  hw_io_port pwr_port;
  hw_io_pin pwr_pin;
  hw_io_port trx_ce_port;
  hw_io_pin trx_ce_pin;
  hw_io_port tx_en_port;
  hw_io_pin tx_en_pin;
} nrf905;

/**
 * Initiates the transceiver in powerdown mode. After this,
 * NRF905_standby should be called; and then the transceiver should
 * be setup using NRF905_config.
 */
void NRF905_init(nrf905 *nrf, spi_dev *spi_dev,
    nrf905_cb cb,
    hw_io_port pwr_port, hw_io_pin pwr_pin,
    hw_io_port trx_ce_port, hw_io_pin trx_ce_pin,
    hw_io_port tx_en_port, hw_io_pin tx_en_pin);

/*
 * This function must be called on high flanks of the DATA READY signal.
 */
void NRF905_signal_data_ready(nrf905 *nrf);

/**
 * Puts the transceiver in powerdown mode.  Can be called
 * from standby mode.
 * Synchronous.
 * States: NRF905_POWERDOWN
 */
int NRF905_powerdown(nrf905 *nrf);

/**
 * Puts the transceiver in standby mode. Can be called
 * from all modes. Will abort anything ongoing.
 * Synchronous.
 * States: NRF905_STANDBY
 */
int NRF905_standby(nrf905 *nrf);

/**
 * Configures the transceiver.
 * Can only be called from standby mode.
 * Returns to standby mode after finished operation.
 * Asynchronous.
 * States: NRF905_CONFIG
 */
int NRF905_config(nrf905 *nrf, nrf905_config *cfg);

/**
 * Reads out transceivers configuration.
 * Can only be called from standby mode.
 * Returns to standby mode after finished operation.
 * Asynchronous.
 * States: NRF905_READ_CONFIG
 */
int NRF905_read_config(nrf905 *nrf, nrf905_config *cfg);

/**
 * Configures the transceivers channel.
 * Can only be called from standby mode.
 * Returns to standby mode after finished operation.
 * Asynchronous.
 * States: NRF905_QUICK_CONFIG
 */
int NRF905_quick_config_channel(nrf905 *nrf, u16_t channel_freq);

/**
 * Configures the transceivers PA.
 * Can only be called from standby mode.
 * Returns to standby mode after finished operation.
 * Asynchronous.
 * States: NRF905_QUICK_CONFIG
 */
int NRF905_quick_config_pa(nrf905 *nrf, nrf905_cfg_pa_pwr pa_pwr);

/**
 * Sets the address to tx to. Must be called prior to a transmit. The
 * same tx address need not to be set prior to all transmits.
 * Can only be called from standby mode.
 * Returns to standby mode after finished operation.
 * Asynchronous.
 * States: NRF905_CONFIG_TX_ADDR
 */
int NRF905_config_tx_address(nrf905 *nrf, u8_t *addr);

/**
 * Transmits a packet.
 * Can only be called from standby mode.
 * Returns to standby mode after finished operation.
 * Asynchronous.
 * States: NRF905_TX_PRIME, NRF905_TX
 */
int NRF905_tx(nrf905 *nrf, u8_t *data, u8_t len);

/**
 * Puts the transceiver in rx mode. When one packet is received,
 * standby mode is returned to.
 * Can only be called from standby mode.
 * Returns to standby mode after finished operation.
 * Asynchronous.
 * States: NRF905_RX_LISTEN, NRF905_RX_READ
 */
int NRF905_rx(nrf905 *nrf);

/**
 * Transmits a carrier.
 * Can only be called from standby mode.
 * Exited when returning to standby mode.
 * Asynchronous.
 * States: NRF905_TX_CARRIER
 */
int NRF905_tx_carrier(nrf905 *nrf);

/**
 * Returns current state of the nrf905 driver.
 */
nrf905_state NRF905_get_state(nrf905 *nrf);

#endif /* NRF905_DRIVER_H_ */

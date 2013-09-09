/*
 * spi_dev.h
 *
 *  Created on: Sep 21, 2012
 *      Author: petera
 */

#ifndef SPI_DEV_H_
#define SPI_DEV_H_

#include "spi_driver.h"
#include "taskq.h"

#define SPI_CONF_IRQ_DRIVEN         (1<<0)
#define SPI_CONF_IRQ_CALLBACK       (1<<1)

#define SPI_ERR_DEV_BUSY            -100


#define SPIDEV_CONFIG_CPOL_MASK     (1<<0)
#define SPIDEV_CONFIG_CPHA_MASK     (1<<1)
#define SPIDEV_CONFIG_FBIT_MASK     (1<<2)
#define SPIDEV_CONFIG_SPEED_MASK    ((0x7)<<3)

#define SPIDEV_CONFIG_CPOL_LO       (1<<0)
#define SPIDEV_CONFIG_CPOL_HI       (0<<0)
#define SPIDEV_CONFIG_CPHA_2E       (1<<1)
#define SPIDEV_CONFIG_CPHA_1E       (0<<1)
#define SPIDEV_CONFIG_FBIT_LSB      (1<<2)
#define SPIDEV_CONFIG_FBIT_MSB      (0<<2)
#define SPIDEV_CONFIG_SPEED_36M     ((0x0)<<3)
#define SPIDEV_CONFIG_SPEED_18M     ((0x1)<<3)
#define SPIDEV_CONFIG_SPEED_9M      ((0x2)<<3)
#define SPIDEV_CONFIG_SPEED_4_5M    ((0x3)<<3)
#define SPIDEV_CONFIG_SPEED_2_3M    ((0x4)<<3)
#define SPIDEV_CONFIG_SPEED_1_1M    ((0x5)<<3)
#define SPIDEV_CONFIG_SPEED_562_5K  ((0x6)<<3)
#define SPIDEV_CONFIG_SPEED_SLOWEST  ((0x7)<<3)


typedef struct  {
  u8_t *tx;
  u8_t cs_release : 1;
  u16_t tx_len : 15;
  u8_t *rx;
  u16_t rx_len;
} __attribute__(( packed )) spi_dev_sequence;

/*
 * SPI device
 */
typedef struct spi_dev_s {
  u16_t configuration;
  spi_bus *bus;
  hw_io_port cs_port;
  hw_io_pin cs_pin;
  bool opened;
  u8_t irq_conf;
  spi_dev_sequence cur_seq;
  spi_dev_sequence *seq_list;
  u8_t seq_len;
  void (*spi_dev_callback)(struct spi_dev_s *s, int result);
} spi_dev;

typedef void (*spi_dev_callback)(spi_dev *dev, int result);

/**
 * Initializes a spi device using given spi bus. The spi device will
 * reconfigure the bus to given configuration when used.
 * @param dev           The device struct
 * @param confiuration  The spi bus configuration used for this device
 * @param bus           The spi bus used by this device
 * @param cs_port       The chip select port
 * @param cs_pin        The chip select pin
 * @param irq_conf      Can be 0, SPI_CONF_IRQ_DRIVEN, or
 *                      SPI_CONF_IRQ_DRIVEN | SPI_CONF_IRQ_CALLBACK.
 *                      0 means all irq calls and further sequencing will result
 *                      in a kernel message, and callback will be done in kernel context.
 *                      SPI_CONF_IRQ_DRIVEN means all sequencing will be performed in
 *                      irq context, but the callback will be called from kernel context.
 *                      SPI_CONF_IRQ_DRIVEN | SPI_CONF_IRQ_CALLBACK will perform all
 *                      sequencing in irq context, and also the callback will be done in
 *                      irq context.
 */
void SPI_DEV_init(spi_dev *dev, u16_t configuration, spi_bus *bus,
    hw_io_port cs_port, hw_io_pin cs_pin, u8_t irq_conf);

/**
 * Opens this spi device for usage.
 * @param dev           The device struct
 */
void SPI_DEV_open(spi_dev *dev);

/**
 * Registers result callback function when spi device operation is done.
 * @param dev     The spi device to operate on
 * @returns   SPI_ERR_DEV_BUSY if device is busy
 *            SPI_OK if all is ok
 */
int SPI_DEV_set_callback(spi_dev *dev, spi_dev_callback cb);

/**
 * Starts a spi device sequence operation.
 * @param dev     The spi device to operate on
 * @param seq     The sequence array to perform
 * @param seq_len The sequence length
 * @returns   SPI_ERR_BUS_BUSY if bus is busy,
 *            SPI_ERR_DEV_BUSY if device is busy
 *            SPI_OK if all is ok
 */
int SPI_DEV_sequence(spi_dev *dev, spi_dev_sequence *seq, u8_t seq_len);

/**
 * Starts a spi device tx/rx operation. If both tx_len and rx_len are greater than zero,
 * tx data is first transmitted, then rx data is received
 * @param dev     The spi device to operate on
 * @param tx      The data to transmit
 * @param tx_len  The length of data to transmit
 * @param rx      The data to receive
 * @param rx_len  The length of data to receive
 * @returns   SPI_ERR_BUS_BUSY if bus is busy,
 *            SPI_ERR_DEV_BUSY if device is busy
 *            SPI_OK if all is ok
 */
int SPI_DEV_txrx(spi_dev *dev, u8_t *tx, u16_t tx_len, u8_t *rx, u16_t rx_len);

/**
 * Closes spi device and deregisters it from the spi bus
 * @param dev     The spi device to operate on
 */
void SPI_DEV_close(spi_dev *dev);

/**
 * Checks if spi device or bus is busy
 * @param dev     The spi device to check
 */
bool SPI_DEV_is_busy(spi_dev *dev);


#endif /* SPI_DEV_H_ */

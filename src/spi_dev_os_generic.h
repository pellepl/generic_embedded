/*
 * spi_dev_os_generic.h
 *
 *  Created on: Mar 27, 2013
 *      Author: petera
 */

#ifndef SPI_DEV_OS_GENERIC_H_
#define SPI_DEV_OS_GENERIC_H_

#include "spi_dev.h"
#include "os.h"

typedef struct {
  spi_dev dev_kernel;
  os_mutex mutex;
  os_cond cond;
  int res;
  volatile bool pending;
} spi_dev_gen;

void SPI_DEV_GEN_init(spi_dev_gen *dev, u16_t configuration, spi_bus *bus,
    hw_io_port cs_port, hw_io_pin cs_pin);

void SPI_DEV_GEN_open(spi_dev_gen *dev);

int SPI_DEV_GEN_sequence(spi_dev_gen *dev, spi_dev_sequence *seq, u8_t seq_len);

int SPI_DEV_GEN_txrx(spi_dev_gen *dev, u8_t *tx, u16_t tx_len, u8_t *rx, u16_t rx_len);

void SPI_DEV_GEN_close(spi_dev_gen *dev);

bool SPI_DEV_GEN_is_busy(spi_dev_gen *dev);

#endif /* SPI_DEV_OS_GENERIC_H_ */

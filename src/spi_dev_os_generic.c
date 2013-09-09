/*
 * spi_dev_os_generic.c
 *
 * Synchronous interface against spi bus. Uses mutexes and conditionals
 * for synchronization, access from user thread.
 *
 *  Created on: Mar 27, 2013
 *      Author: petera
 */


#include "spi_dev_os_generic.h"
#include "miniutils.h"

#ifdef CONFIG_SPI

static void SPI_DEV_GEN_cb(spi_dev *cb_dev_kernel, int result) {
  spi_dev_gen *dev = (spi_dev_gen *)((void*)cb_dev_kernel - offsetof(spi_dev_gen, dev_kernel));
  dev->res = result;
  dev->pending = FALSE;
  DBG(D_SPI, D_DEBUG, "SPI_DEV_GEN irq cb res %i\n", result);
#ifndef CONFIG_SPI_POLL
  OS_cond_signal(&dev->cond);
#endif
}

void SPI_DEV_GEN_init(spi_dev_gen *dev, u16_t configuration, spi_bus *bus,
    hw_io_port cs_port, hw_io_pin cs_pin) {
  SPI_DEV_init(&dev->dev_kernel, configuration, bus, cs_port, cs_pin, SPI_CONF_IRQ_CALLBACK | SPI_CONF_IRQ_DRIVEN);
#ifndef CONFIG_SPI_POLL
  OS_mutex_init(&dev->mutex, OS_MUTEX_ATTR_CRITICAL_IRQ);
  OS_cond_init(&dev->cond);
#endif
  SPI_DEV_set_callback(&dev->dev_kernel, SPI_DEV_GEN_cb);
}

void SPI_DEV_GEN_open(spi_dev_gen *dev) {
  SPI_DEV_open(&dev->dev_kernel);
}

int SPI_DEV_GEN_sequence(spi_dev_gen *dev, spi_dev_sequence *seq, u8_t seq_len) {
  int res;
#ifndef CONFIG_SPI_POLL
  OS_mutex_lock(&dev->mutex);
#endif
  dev->pending = TRUE;
  res = SPI_DEV_sequence(&dev->dev_kernel, seq, seq_len);
  if (res != SPI_OK) {
    dev->pending = FALSE;
#ifndef CONFIG_SPI_POLL
    OS_mutex_unlock(&dev->mutex);
#endif
    return res;
  }
#ifndef CONFIG_SPI_POLL
  DBG(D_SPI, D_DEBUG, "SPI_DEV_GEN await\n");
  while (dev->pending) {
    OS_cond_wait(&dev->cond, &dev->mutex);
  }
#endif
  res = dev->res;
  DBG(D_SPI, D_DEBUG, "SPI_DEV_GEN release, res %i\n", res);
#ifndef CONFIG_SPI_POLL
  OS_mutex_unlock(&dev->mutex);
#endif
  return res;
}

int SPI_DEV_GEN_txrx(spi_dev_gen *dev, u8_t *tx, u16_t tx_len, u8_t *rx, u16_t rx_len) {
  int res;
#ifndef CONFIG_SPI_POLL
  OS_mutex_lock(&dev->mutex);
#endif
  dev->pending = TRUE;
  res = SPI_DEV_txrx(&dev->dev_kernel, tx, tx_len, rx, rx_len);
  if (res != SPI_OK) {
    dev->pending = FALSE;
#ifndef CONFIG_SPI_POLL
    OS_mutex_unlock(&dev->mutex);
#endif
    return res;
  }
#ifndef CONFIG_SPI_POLL
  DBG(D_SPI, D_DEBUG, "SPI_DEV_GEN await\n");
  while (dev->pending) {
    OS_cond_wait(&dev->cond, &dev->mutex);
  }
#endif
  res = dev->res;
  DBG(D_SPI, D_DEBUG, "SPI_DEV_GEN release, res %i\n", res);
#ifndef CONFIG_SPI_POLL
  OS_mutex_unlock(&dev->mutex);
#endif
  return res;
}

void SPI_DEV_GEN_close(spi_dev_gen *dev) {
  SPI_DEV_close(&dev->dev_kernel);
}

bool SPI_DEV_GEN_is_busy(spi_dev_gen *dev) {
  return SPI_DEV_is_busy(&dev->dev_kernel);
}

#endif

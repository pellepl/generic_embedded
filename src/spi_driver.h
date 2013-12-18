/*
 * spi_driver.h
 *
 *  Created on: Aug 15, 2012
 *      Author: petera
 */

#ifndef SPI_DRIVER_H_
#define SPI_DRIVER_H_

#include "system.h"

#define SPI_BUFFER    256

#define SPI_OK                    0
#define SPI_ERR_BUS_BUSY          -2000
#define SPI_ERR_BUS_LEN_EXCEEDED  -2001
#define SPI_ERR_BUS_PHY           -2002

/*
 * SPI bus
 */
typedef struct spi_bus_s {
#ifdef ARCH_STM32
  SPI_TypeDef *hw;
#if defined(PROC_STM32F1)
  DMA_Channel_TypeDef *dma_rx_stream;
  DMA_Channel_TypeDef *dma_tx_stream;
  u32_t dma_rx_irq;
  u32_t dma_tx_irq;
  u32_t dma_rx_err_irq;
  u32_t dma_tx_err_irq;
#elif defined(PROC_STM32F4)
  DMA_Stream_TypeDef *dma_rx_stream;
  DMA_Stream_TypeDef *dma_tx_stream;
  u32_t dma_channel;
  u32_t dma_rx_irq;
  u32_t dma_tx_irq;
  u32_t dma_rx_err_dm_irq;
  u32_t dma_tx_err_dm_irq;
  u32_t dma_rx_err_tr_irq;
  u32_t dma_tx_err_tr_irq;
#endif
#endif
  u8_t attached_devices;
  volatile bool busy;
  u8_t buf[SPI_BUFFER];
  u16_t max_buf_len;
  u8_t dummy;
  u8_t *rx_buf;
  u16_t rx_len;
  void (*spi_bus_callback)(struct spi_bus_s *s, s32_t res);
  void *user_p;
  volatile u32_t user_arg;
} spi_bus;

#if defined(CONFIG_SPI1) && defined(CONFIG_SPI2)
#define SPI_MAX_ID   2
#elif defined(CONFIG_SPI1) || defined(CONFIG_SPI2)
#define SPI_MAX_ID   1
#else
#define SPI_MAX_ID   0
#endif

extern spi_bus __spi_bus_vec[SPI_MAX_ID];

#define _SPI_BUS(x) (&__spi_bus_vec[(x)])

/* Receives data from spi bus. The length here is up to the user.
 * @return SPI_OK or error
 */
int SPI_rx(spi_bus *s, u8_t *rx, u16_t rx_len);

/* Sends data on spi bus
 * @param tx_len length to tx, must not exceed internal buffer length of driver
 * @return SPI_OK or error
 */
int SPI_tx(spi_bus *s, u8_t *tx, u16_t tx_len);

/* Sends and receives data on spi bus, full duplex
 * @param tx_len length to tx, must not exceed internal buffer length of driver
 * @param rx_len length to rx, must not exceed internal buffer length of driver
 * @return SPI_OK or error
 */
int SPI_rxtx(spi_bus *s, u8_t *tx, u16_t tx_len, u8_t *rx, u16_t rx_len);

/* Closes indefinitely.
 * @return SPI_ERR_BUSY if bus was busy, but still it is closed.
 */
int SPI_close(spi_bus *spi);

/* Sets a callback which is invoked upon following rx/tx completions.
 * Called directly after bus is freed and finalized in irq context.
 * @return SPI_OK or SPI_ERR_BUSY if bus is currently busy
 */
int SPI_set_callback(spi_bus *spi, void (*spi_bus_callback)(spi_bus *s, s32_t res));

/**
 * A call to spi config will reset bus, configure it and then (re)start the
 * SPI clock
 * @return SPI_OK
 */
int SPI_config(spi_bus *s, u16_t config);

/**
 * Returns if bus is busy or not
 */
bool SPI_is_busy(spi_bus *spi);

/**
 * Register a user of the spi bus
 */
void SPI_register(spi_bus *spi);
/**
 * Deregister a user of the spi bus, if reaching zero the spi bus is closed
 */
void SPI_release(spi_bus *spi);

/**
 * IRQ handler for spi buses
 */
void SPI_irq(spi_bus *spi);

/**
 * Initializes spi buses
 */
void SPI_init();

#endif /* SPI_DRIVER_H_ */

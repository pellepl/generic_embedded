/*
 * spi_dev.c
 *
 *  Created on: Sep 21, 2012
 *      Author: petera
 */
#include "spi_dev.h"
#include "miniutils.h"
#include "system.h"

#ifdef CONFIG_SPI

/*
 * NB: spi_bus struct user_pointer holds device struct spi_dev
 */

#define SPI_DEV_BUS_USER_ARG_BUSY_BIT       (1<<0)

static void SPI_DEV_task_f_finish(u32_t res, void *spi_dev_v);
static void SPI_DEV_task_f_next(u32_t res, void *spi_dev_v);

// Reconfigure spi hw block
static void SPI_DEV_config(spi_bus *spi, u16_t config) {
  DBG(D_SPI, D_DEBUG, "SPI DEV reconfig %04x\n", config);
  SPI_config(spi, config);
}

// Assert/deassert chip select
static void SPI_DEV_cs(spi_dev *dev, bool enable) {
  if (enable) {
    DBG(D_SPI, D_DEBUG, "SPI DEV cs on\n");
    GPIO_disable(dev->cs_port, dev->cs_pin);
  } else {
    DBG(D_SPI, D_DEBUG, "SPI DEV cs off\n");
    GPIO_enable(dev->cs_port, dev->cs_pin);
  }
}

// Reset spi device and deasserts chip select
static void SPI_DEV_reset(spi_dev *dev) {
  dev->cur_seq.tx_len = 0;
  dev->cur_seq.rx_len = 0;
  dev->seq_len = 0;
  dev->bus->user_arg &= ~SPI_DEV_BUS_USER_ARG_BUSY_BIT;
  SPI_DEV_cs(dev, FALSE);
}

// Invoked when spi tx/rx operation is finished. Resets spi device.
// If irq driven & irq callback, the registered callback is done in irq
// If irq driven & !irq callback, the registered callback is done in kernel task thread
// If !irq driven, the registered callback is done in kernel task thread
static void SPI_DEV_finish(spi_dev *dev, int res) {
  if (res == SPI_OK) {
    DBG(D_SPI, D_DEBUG, "SPI DEV fin OK   res:%i CS off\n", res);
  } else {
    DBG(D_SPI, D_WARN, "SPI DEV fin FAIL res:%i CS off\n", res);
  }
  SPI_DEV_reset(dev);

  if (dev->irq_conf & SPI_CONF_IRQ_DRIVEN) {
    // finished all in irq
    if (dev->irq_conf & SPI_CONF_IRQ_CALLBACK) {
      // use irq context to call user callback function
      if (dev->spi_dev_callback) {
        dev->spi_dev_callback(dev, res);
      }
    } else {
      // use task to call user callback function in task ctx
      task *t = TASK_create(SPI_DEV_task_f_finish, 0);
      ASSERT(t != NULL);
      TASK_run(t, res, dev);
    }
  } else {
    // finished all in task context, simply call user callback
    if (dev->spi_dev_callback) {
      dev->spi_dev_callback(dev, res);
    }
  }
}

// Executes pending rx/tx operation or next sequence. Finalizes
// if there are no rx/tx data left and no sequences.
static void SPI_DEV_exec(spi_dev *dev) {
  ASSERT(dev != NULL);

  dev->bus->user_p = dev;
  bool cs_released = FALSE;
  if (dev->cur_seq.tx_len == 0 && dev->cur_seq.rx_len == 0) {
    // this sequence is finished
    cs_released = dev->cur_seq.cs_release;
    if (cs_released) {
      // this finished sequence wanted us to release CS
      SPI_DEV_cs(dev, FALSE);
    }
    // check for more sequences in list
    if (dev->seq_len > 0) {
      DBG(D_SPI, D_DEBUG, "SPI DEV exe next sequence seqs:%04x\n", dev->seq_len);
      memcpy(&dev->cur_seq, dev->seq_list, sizeof(spi_dev_sequence));
      dev->seq_len--;
      dev->seq_list++;
    } else {
      // finished
      DBG(D_SPI, D_DEBUG, "SPI DEV exe finish OK\n");
      SPI_DEV_finish(dev, SPI_OK);
      return;
    }
  }
  DBG(D_SPI, D_DEBUG, "SPI DEV exe   [txl:%04x rxl:%04x] released_cs:%1x will_release_cs:%1x\n",
      dev->cur_seq.tx_len, dev->cur_seq.rx_len, cs_released, dev->cur_seq.cs_release);
  int res;
  if (dev->cur_seq.tx_len > 0) {
    // something to send
    if (cs_released) {
      SPI_DEV_cs(dev, TRUE);
    }
    // chunk too big things up
    u16_t len_to_tx = MIN(dev->cur_seq.tx_len, dev->bus->max_buf_len);
    u8_t *tx_buf = dev->cur_seq.tx;
    DBG(D_SPI, D_DEBUG, "SPI DEV exe   tx %04x / %04x\n", len_to_tx, dev->cur_seq.tx_len);
    dev->cur_seq.tx_len -= len_to_tx;
    dev->cur_seq.tx += len_to_tx;
    res = SPI_tx(dev->bus, tx_buf, len_to_tx);
    if (res != SPI_OK) {
      SPI_DEV_finish(dev, res);
    }
  } else if (dev->cur_seq.rx_len > 0) {
    // something to receive
    if (cs_released) {
      SPI_DEV_cs(dev, TRUE);
    }
    u16_t rx_len = dev->cur_seq.rx_len;
    DBG(D_SPI, D_DEBUG, "SPI DEV exe   rx %04x\n", dev->cur_seq.rx_len);
    dev->cur_seq.rx_len = 0;
    res = SPI_rx(dev->bus, dev->cur_seq.rx, rx_len);
    if (res != SPI_OK) {
      SPI_DEV_finish(dev, res);
    }
  }
}

// When spi device is irq driven & !irq callback, this task function calls the
// callback
static void SPI_DEV_task_f_finish(u32_t res, void *spi_dev_v) {
  spi_dev *dev = (spi_dev*)spi_dev_v;
  // if doing all in irq, this task calls the device caller callback func
  if (dev->spi_dev_callback) {
    dev->spi_dev_callback(dev, res);
  }
}

// When spi device is !irq driven, this task function executes next pending operation
static void SPI_DEV_task_f_next(u32_t res, void *spi_dev_v) {
  spi_dev *dev = (spi_dev*)spi_dev_v;
  // if not doing all in irq, use task to exec next spi step
  // disable spi irqs while processing next spi step
  //SPI_disable_irq(dev->bus);

  if (res == SPI_OK) {
    SPI_DEV_exec(dev);
  } else {
    SPI_DEV_finish(dev, res);
  }

  // reenable spi irqs after processed next spi step
  //SPI_enable_irq(dev->bus);
}

// Spi device irq callback
static void SPI_DEV_callback_irq(spi_bus *spi, s32_t res) {
  ASSERT(spi != NULL);
  spi_dev *dev = (spi_dev *)spi->user_p;
  ASSERT(dev != NULL);
  if (dev->irq_conf & SPI_CONF_IRQ_DRIVEN) {
    // if doing all in irq, use irq context to exec next spi step
    if (res == SPI_OK) {
      SPI_DEV_exec(dev);
    } else {
      SPI_DEV_finish(dev, res);
    }
  } else {
    // if not doing all in irq, start our task to execute next spi step in task context
    if (dev->spi_dev_callback) {
      task *t = TASK_create(SPI_DEV_task_f_next, 0);
      ASSERT(t != NULL);
      TASK_run(t, res, dev);
    }
  }
}

int SPI_DEV_set_callback(spi_dev *dev, spi_dev_callback cb) {
  if (dev->bus->user_arg & SPI_DEV_BUS_USER_ARG_BUSY_BIT) {
    return SPI_ERR_DEV_BUSY;
  }
  dev->spi_dev_callback = cb;
  return SPI_OK;
}

int SPI_DEV_set_user_data(spi_dev *dev, void *user_data) {
  if (dev->bus->user_arg & SPI_DEV_BUS_USER_ARG_BUSY_BIT) {
    return SPI_ERR_DEV_BUSY;
  }
  dev->user_data = user_data;
  return SPI_OK;
}

int SPI_DEV_sequence(spi_dev *dev, spi_dev_sequence *seq, u8_t seq_len) {
  if (dev->bus->busy) {
    return SPI_ERR_BUS_BUSY;
  }
  if (dev->bus->user_arg & SPI_DEV_BUS_USER_ARG_BUSY_BIT) {
    return SPI_ERR_DEV_BUSY;
  }

  dev->bus->user_arg |= SPI_DEV_BUS_USER_ARG_BUSY_BIT;

  if (dev->bus->user_p == NULL || ((spi_dev *)dev->bus->user_p)->configuration != dev->configuration) {
    // last bus use wasn't this device, so reconfigure
    SPI_DEV_config(dev->bus, dev->configuration);
  }
  dev->seq_len = seq_len;
  dev->seq_list = seq;
  dev->cur_seq.tx_len = 0;
  dev->cur_seq.rx_len = 0;
  dev->cur_seq.cs_release = 0;
  SPI_DEV_cs(dev, TRUE);
  SPI_DEV_exec(dev);

  return SPI_OK;
}

int SPI_DEV_txrx(spi_dev *dev, u8_t *tx, u16_t tx_len, u8_t *rx, u16_t rx_len) {
  if (dev->bus->busy) {
    return SPI_ERR_BUS_BUSY;
  }
  if (dev->bus->user_arg & SPI_DEV_BUS_USER_ARG_BUSY_BIT) {
    return SPI_ERR_DEV_BUSY;
  }

  dev->bus->user_arg |= SPI_DEV_BUS_USER_ARG_BUSY_BIT;

  if (dev->bus->user_p != dev) {
    // last bus use wasn't this device, so reconfigure
    SPI_DEV_config(dev->bus, dev->configuration);
  }
  dev->seq_len = 0;
  dev->cur_seq.tx = tx;
  dev->cur_seq.tx_len = tx_len;
  dev->cur_seq.rx = rx;
  dev->cur_seq.rx_len = rx_len;
  dev->cur_seq.cs_release = TRUE;
  SPI_DEV_cs(dev, TRUE);
  SPI_DEV_exec(dev);

  return SPI_OK;
}

void SPI_DEV_init(spi_dev *dev, u16_t configuration, spi_bus *bus,
    hw_io_port cs_port, hw_io_pin cs_pin, u8_t irq_conf) {
  memset(dev, 0, sizeof(spi_dev));
  dev->configuration = configuration;
  dev->bus = bus;
  dev->cs_port = cs_port;
  dev->cs_pin = cs_pin;
  dev->irq_conf = irq_conf;

  SPI_DEV_cs(dev, FALSE);

  SPI_set_callback(dev->bus, SPI_DEV_callback_irq);
}

bool SPI_DEV_is_busy(spi_dev *dev) {
  return SPI_is_busy(dev->bus) | ((dev->bus->user_arg & SPI_DEV_BUS_USER_ARG_BUSY_BIT) != 0);
}


void SPI_DEV_close(spi_dev *dev) {
  if (dev->opened) {
    SPI_release(dev->bus);
    SPI_DEV_reset(dev);
    dev->opened = FALSE;
  }
}

void SPI_DEV_open(spi_dev *dev) {
  if (!dev->opened) {
    SPI_register(dev->bus);
    SPI_DEV_reset(dev);
    dev->opened = TRUE;
  }
}

#endif

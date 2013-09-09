/*
 * spi_flash.c
 *
 *  Created on: Sep 23, 2012
 *      Author: petera
 */

#include "spi_flash.h"
#include "spi_dev.h"
#include "taskq.h"
#include "miniutils.h"
#include "led.h"

#ifdef CONFIG_SPI

static int spi_flash_send_write_sequence(spi_flash_dev *sfd);
static int spi_flash_send_erase_sequence(spi_flash_dev *sfd);

static int spi_flash_exec(spi_flash_dev *sfd, spi_dev_sequence *seq, u8_t seq_len) {
  LED_blink_single(LED_SPI_FLASH_BIT, 2,1, LED_BLINK_FOREVER);
  return SPI_DEV_sequence(&sfd->dev, seq, seq_len);
}

static void spi_flash_finalize(spi_flash_dev *sfd, int res) {
  LED_blink_single(LED_SPI_FLASH_BIT, 2,1,2);
  if (res != SPI_OK) {
    LED_blink_single(LED_ERROR1_BIT, 6, 4, 5);
  }
  sfd->busy = FALSE;
  if (sfd->spi_flash_callback) {
    sfd->spi_flash_callback(sfd, res);
  }
}

static void spi_flash_call_task_within_time(spi_flash_dev *sfd, u32_t time, int res) {
  if (sfd->flash_conf.busy_poll_divisor > 0) {
    u32_t poll_time = time / sfd->flash_conf.busy_poll_divisor;
    if (poll_time < sfd->flash_conf.busy_poll_min_time) {
      poll_time = sfd->flash_conf.busy_poll_min_time;
    }

    sfd->busy_poll = TRUE;
    sfd->poll_count = 0;
    TASK_start_timer(sfd->task, &sfd->timer, res, sfd, poll_time, poll_time, "spif_poll");
  } else {
    sfd->busy_poll = FALSE;
    TASK_start_timer(sfd->task, &sfd->timer, res, sfd, time, 0, "spif_cb");
  }
}

static void spi_flash_set_first_seq_to_wren(spi_flash_dev *sfd) {
  sfd->sequence_buf[0].tx = (u8_t*)&sfd->flash_conf.cmd_defs[SPI_FLASH_CMD_WRITE_ENABLE];
  sfd->sequence_buf[0].tx_len = 1;
  sfd->sequence_buf[0].rx = 0;
  sfd->sequence_buf[0].rx_len = 0;
  sfd->sequence_buf[0].cs_release = 1;
}

static void spi_flash_fill_in_address(spi_flash_dev *sfd, u8_t *dst, u32_t addr) {
  int bytes = sfd->flash_conf.addressing_bytes + 1;
  int i;
  for (i = 0; i < bytes; i++) {
    if (sfd->flash_conf.addressing_endianess) {
      dst[i] = (addr & 0xff);
    } else {
      dst[bytes - 1 - i] = (addr & 0xff);
    }
    addr >>= 8;
  }
}

/*
 * SPI state spinner, called from spi device finished callback
 */
static void spi_flash_update_state(spi_flash_dev *sfd) {
  int res = SPI_OK;

  if (sfd->busy_poll) {
    // polling busy bit
    u32_t busy_res = sfd->tmp_buf[0] & (1<<sfd->flash_conf.busy_sr_bit);
    bool poll_exceed = sfd->poll_count > sfd->flash_conf.busy_poll_divisor;
    if (!poll_exceed && busy_res) {
      DBG(D_SPI, D_DEBUG, "SPIF poll: still busy\n");
      // still busy, return
      return;
    } else {
      // not busy any longer or timeout, continue state switching
      if (poll_exceed) {
        DBG(D_SPI, D_WARN, "SPIF poll: poll count exceeded\n");
      } else {
        DBG(D_SPI, D_DEBUG, "SPIF poll: busy released\n");
      }
      TASK_stop_timer(&sfd->timer);
      sfd->busy_poll = FALSE;
      TASK_run(sfd->task, res, sfd);
      return;
    }
  }

  switch (sfd->state) {

  // opening
  case SPI_FLASH_STATE_OPENING_READ_ID: {
    u32_t ret_id =
        sfd->tmp_buf[2] | (sfd->tmp_buf[1] << 8) | (sfd->tmp_buf[0] << 16);
    if (ret_id != sfd->flash_conf.flash_id) {
      DBG(D_SPI, D_WARN, "SPIF invalid id %08x != %08x\n", ret_id, sfd->flash_conf.flash_id);
      res = SPI_FLASH_ERR_INVALID_ID;
    } else {
      sfd->state = SPI_FLASH_STATE_OPENED;
      sfd->open = TRUE;
      TASK_run(sfd->task, res, sfd);
    }
  }
  break;

  // close / protect
  case SPI_FLASH_STATE_CLOSING:
    sfd->state = SPI_FLASH_STATE_CLOSED;
    spi_flash_call_task_within_time(sfd, sfd->flash_conf.time_sr_write_ms, res);
    break;

  // read sr
  case SPI_FLASH_STATE_READ_BUSY: {
    sfd->state = SPI_FLASH_STATE_OPENED;
    u8_t busy_res = sfd->sr_tmp & (1<<sfd->flash_conf.busy_sr_bit);
    if (sfd->sr_dst) {
      *((u8_t*)sfd->sr_dst) = busy_res;
      sfd->sr_dst = 0;
    }
    TASK_run(sfd->task, res, sfd);
  }
  break;

  // read data
  case SPI_FLASH_STATE_READ:
    sfd->state = SPI_FLASH_STATE_OPENED;
    TASK_run(sfd->task, res, sfd);
    break;
  // write data
  case SPI_FLASH_STATE_WRITE_SEQ:
    sfd->state = SPI_FLASH_STATE_WRITE_WAIT;
    spi_flash_call_task_within_time(sfd, sfd->flash_conf.time_page_write_ms, res);
    break;

  // erase sector
  case SPI_FLASH_STATE_ERASE_SEQ:
    sfd->state = SPI_FLASH_STATE_ERASE_WAIT;
    spi_flash_call_task_within_time(sfd, sfd->flash_conf.time_sector_erase_ms, res);
    break;

  // protect/unprotect
  case SPI_FLASH_STATE_WRSR_SEQ:
    sfd->state = SPI_FLASH_STATE_WRSR_WAIT;
    spi_flash_call_task_within_time(sfd, sfd->flash_conf.time_sr_write_ms, res);
    break;

  // mass erase
  case SPI_FLASH_STATE_MASS_ERASE:
    sfd->state = SPI_FLASH_STATE_OPENED;
    spi_flash_call_task_within_time(sfd, sfd->flash_conf.time_mass_erase_ms, res);
    break;

  // other
  case SPI_FLASH_STATE_ERROR:
  case SPI_FLASH_STATE_OPENED:
  case SPI_FLASH_STATE_CLOSED:
    // noop
    break;

  default:
    res = SPI_FLASH_ERR_UNDEFINED_STATE;
    break;
  }

  // something went wrong, report error
  if (res != SPI_OK) {
    if (sfd->busy_poll) {
      TASK_stop_timer(&sfd->timer);
      sfd->busy_poll = FALSE;
    }
    sfd->state = SPI_FLASH_STATE_ERROR;
    TASK_run(sfd->task, res, sfd);
  }
}

/*
 * spi device result callback (might be called from irq, flag)
 */
static void spi_flash_callback_spi_result(spi_dev *dev, int res) {
  spi_flash_dev *sfd = (spi_flash_dev *)((char*)dev - offsetof(spi_flash_dev, dev));
  if (res != SPI_OK) {
    DBG(D_SPI, D_WARN, "SPIF cb err i\n", res);
    if (sfd->busy_poll) {
      TASK_stop_timer(&sfd->timer);
      sfd->busy_poll = FALSE;
    }
    sfd->state = SPI_FLASH_STATE_ERROR;
    TASK_run(sfd->task, res, sfd);
    return;
  }
  spi_flash_update_state(sfd);
}

/*
 * spi flash task function, used for reporting finished operations to user and
 * to keep state of intermediate operations
 */
static void spi_flash_task_f(u32_t res_u, void *sfd_v) {

  int res = (int)res_u;
  spi_flash_dev *sfd = (spi_flash_dev *)sfd_v;
  ASSERT(sfd);

  if (sfd->busy_poll) {
    spi_dev_sequence seq;
    // busy poll, fire off an SR read

    sfd->poll_count++;
    DBG(D_SPI, D_DEBUG, "SPIF poll: SR busy %i/%i?\n", sfd->poll_count, sfd->flash_conf.busy_poll_divisor);
    seq.tx = (u8_t*)&sfd->flash_conf.cmd_defs[SPI_FLASH_CMD_READ_SR];
    seq.tx_len = 1;
    seq.rx = sfd->tmp_buf;
    seq.rx_len = 1;
    seq.cs_release = 0;
    res = spi_flash_exec(sfd, &seq, 1);
    if (res == SPI_ERR_DEV_BUSY && sfd->poll_count < 16) {
      DBG(D_SPI, D_INFO, "SPIF poll: device busy\n");
    } else if (res != SPI_OK) {
      DBG(D_SPI, D_WARN, "SPIF poll: ERROR %i\n", res);
      TASK_stop_timer(&sfd->timer);
      sfd->busy_poll = FALSE;

      sfd->state = SPI_FLASH_STATE_ERROR;
      spi_flash_finalize(sfd, res);
    }
    return;
  }

  // if res != SPI_OK, state is always SPI_FLASH_STATE_ERROR here

  switch (sfd->state) {
  case SPI_FLASH_STATE_WRITE_WAIT: {
    if (sfd->count > 0) {
      // more to write
      sfd->state = SPI_FLASH_STATE_WRITE_SEQ;
      res = spi_flash_send_write_sequence(sfd);
      if (res != SPI_OK) {
        // error in middle of write
        sfd->state = SPI_FLASH_STATE_ERROR;
        spi_flash_finalize(sfd, res);
      }
    } else {
      // finished
      sfd->state = SPI_FLASH_STATE_OPENED;
      spi_flash_finalize(sfd, res);
    }
    break;
  }
  case SPI_FLASH_STATE_ERASE_WAIT: {
    if (sfd->count > 0) {
      // more to erase
      sfd->state = SPI_FLASH_STATE_ERASE_SEQ;
      res = spi_flash_send_erase_sequence(sfd);
      if (res != SPI_OK) {
        // error in middle of erase
        sfd->state = SPI_FLASH_STATE_ERROR;
        spi_flash_finalize(sfd, res);
      }
    } else {
      // finished
      sfd->state = SPI_FLASH_STATE_OPENED;
      spi_flash_finalize(sfd, res);
    }
    break;
  }
  case SPI_FLASH_STATE_CLOSED:
    if (sfd->open) {
      SPI_DEV_close(&sfd->dev);
      spi_flash_finalize(sfd, res);
      TASK_stop_timer(&sfd->timer);
      TASK_free(sfd->task);
      sfd->open = FALSE;
    }
  break;
  case SPI_FLASH_STATE_WRSR_WAIT:
  case SPI_FLASH_STATE_OPENED:
  case SPI_FLASH_STATE_ERROR:
    spi_flash_finalize(sfd, res);
  break;
  default:
    sfd->state = SPI_FLASH_STATE_ERROR;
    if (sfd->open) {
      SPI_DEV_close(&sfd->dev);
      spi_flash_finalize(sfd, res);
      TASK_stop_timer(&sfd->timer);
      TASK_free(sfd->task);
      sfd->open = FALSE;
    }
    break;
  }
}

int SPI_FLASH_open(spi_flash_dev *sfd, spi_flash_callback cb) {
  if (sfd->open) {
    return SPI_FLASH_ERR_OPENED;
  }
  if (sfd->busy) {
    return SPI_FLASH_ERR_BUSY;
  }
  sfd->busy = TRUE;

  sfd->state = SPI_FLASH_STATE_OPENING_READ_ID;
  sfd->sequence_buf[0].tx = (u8_t*)&sfd->flash_conf.cmd_defs[SPI_FLASH_CMD_READ_ID];
  sfd->sequence_buf[0].tx_len = 1;
  sfd->sequence_buf[0].rx = sfd->tmp_buf;
  sfd->sequence_buf[0].rx_len = 3;

  sfd->sequence_buf[0].cs_release = 0;
  sfd->spi_flash_callback = cb;

  return spi_flash_exec(sfd, &sfd->sequence_buf[0], 1);
}

static int spi_flash_closeprotectun(spi_flash_dev *sfd, char unprotect) {
  spi_flash_set_first_seq_to_wren(sfd);

  sfd->tmp_buf[0] = sfd->flash_conf.cmd_defs[SPI_FLASH_CMD_WRITE_SR];
  sfd->tmp_buf[1] = sfd->flash_conf.cmd_defs[unprotect ? SPI_FLASH_CMD_SR_UNPROTECT : SPI_FLASH_CMD_SR_PROTECT];
  sfd->sequence_buf[1].tx = &sfd->tmp_buf[0];
  sfd->sequence_buf[1].tx_len = 2;
  sfd->sequence_buf[1].rx = 0;
  sfd->sequence_buf[1].rx_len = 0;
  sfd->sequence_buf[1].cs_release = 0;

  return spi_flash_exec(sfd, &sfd->sequence_buf[0], 2);
}

int SPI_FLASH_close(spi_flash_dev *sfd, spi_flash_callback cb) {
  if (!sfd->open) {
    return SPI_FLASH_ERR_CLOSED;
  }
  if (sfd->busy) {
    return SPI_FLASH_ERR_BUSY;
  }
  sfd->busy = TRUE;
  sfd->state = SPI_FLASH_STATE_CLOSING;
  sfd->spi_flash_callback = cb;

  return spi_flash_closeprotectun(sfd, FALSE);
}

int SPI_FLASH_close_force(spi_flash_dev *sfd) {
  SPI_close(sfd->dev.bus);
  SPI_DEV_close(&sfd->dev);
  sfd->state = SPI_FLASH_STATE_CLOSED;
  spi_flash_finalize(sfd, SPI_OK);
  TASK_stop_timer(&sfd->timer);
  TASK_free(sfd->task);

  return SPI_OK;
}

int SPI_FLASH_read_busy(spi_flash_dev *sfd, spi_flash_callback cb, u8_t *res) {
  spi_dev_sequence seq;
  sfd->state = SPI_FLASH_STATE_READ_BUSY;
  sfd->sr_dst = res;
  seq.tx = (u8_t*)&sfd->flash_conf.cmd_defs[SPI_FLASH_CMD_READ_SR];
  seq.tx_len = 1;
  seq.rx = &sfd->sr_tmp;
  seq.rx_len = 1;
  seq.cs_release = 0;
  sfd->spi_flash_callback = cb;

  return spi_flash_exec(sfd, &seq, 1);
}

int SPI_FLASH_read(spi_flash_dev *sfd, spi_flash_callback cb, u32_t addr, u16_t size, u8_t *dest) {
  if (!sfd->open) {
    return SPI_FLASH_ERR_CLOSED;
  }
  if (addr > sfd->flash_conf.size_total || (addr+size) > sfd->flash_conf.size_total) {
    return SPI_FLASH_ERR_ADDRESS;
  }
  if (sfd->busy) {
    return SPI_FLASH_ERR_BUSY;
  }
  sfd->busy = TRUE;
  sfd->state = SPI_FLASH_STATE_READ;

  sfd->tmp_buf[0] = sfd->flash_conf.cmd_defs[SPI_FLASH_CMD_READ];
  spi_flash_fill_in_address(sfd, &sfd->tmp_buf[1], (u32_t)addr);
  sfd->sequence_buf[0].tx = &sfd->tmp_buf[0];
  sfd->sequence_buf[0].tx_len = 1 + (1+sfd->flash_conf.addressing_bytes);
  sfd->sequence_buf[0].rx = dest;
  sfd->sequence_buf[0].rx_len = size;
  sfd->sequence_buf[0].cs_release = 0;
  sfd->spi_flash_callback = cb;

  return spi_flash_exec(sfd, &sfd->sequence_buf[0], 1);
}

static int spi_flash_send_write_sequence(spi_flash_dev *sfd) {
  u32_t addr = sfd->addr;
  u32_t len_rem_page = sfd->flash_conf.size_page_write_max - (addr & (sfd->flash_conf.size_page_write_max - 1));
  u32_t len_to_write = MIN(len_rem_page, sfd->count);
  if (len_to_write == 0) {
    len_to_write = MIN(sfd->count, sfd->flash_conf.size_page_write_max);
  }

  spi_flash_set_first_seq_to_wren(sfd);

  sfd->tmp_buf[0] = sfd->flash_conf.cmd_defs[SPI_FLASH_CMD_WRITE];
  spi_flash_fill_in_address(sfd, &sfd->tmp_buf[1], addr);
  sfd->sequence_buf[1].tx = &sfd->tmp_buf[0];
  sfd->sequence_buf[1].tx_len = 1 + sfd->flash_conf.addressing_bytes + 1;
  sfd->sequence_buf[1].rx = 0;
  sfd->sequence_buf[1].rx_len = 0;
  sfd->sequence_buf[1].cs_release = 0;

  sfd->sequence_buf[2].tx = (u8_t*)sfd->ptr;
  sfd->sequence_buf[2].tx_len = len_to_write;
  sfd->sequence_buf[2].rx = 0;
  sfd->sequence_buf[2].rx_len = 0;
  sfd->sequence_buf[2].cs_release = 0;

  int res = spi_flash_exec(sfd, &sfd->sequence_buf[0], 3);

  if (res == SPI_OK) {
    sfd->addr += len_to_write;
    sfd->ptr += len_to_write;
    sfd->count -= len_to_write;
  }

  return res;
}

int SPI_FLASH_write(spi_flash_dev *sfd, spi_flash_callback cb, u32_t addr, u16_t size, u8_t *src) {
  if (!sfd->open) {
    return SPI_FLASH_ERR_CLOSED;
  }
  if (addr > sfd->flash_conf.size_total || (addr+size) > sfd->flash_conf.size_total) {
    return SPI_FLASH_ERR_ADDRESS;
  }
  if (sfd->busy) {
    return SPI_FLASH_ERR_BUSY;
  }
  sfd->busy = TRUE;
  sfd->state = SPI_FLASH_STATE_WRITE_SEQ;

  sfd->addr = addr;
  sfd->ptr = src;
  sfd->count = size;
  sfd->spi_flash_callback = cb;

  return spi_flash_send_write_sequence(sfd);
}

static int spi_flash_send_erase_sequence(spi_flash_dev *sfd) {
  u32_t addr = sfd->addr;
  u32_t len = sfd->flash_conf.size_sector_erase_min;

  spi_flash_set_first_seq_to_wren(sfd);

  sfd->tmp_buf[0] = sfd->flash_conf.cmd_defs[SPI_FLASH_CMD_SECTOR_ERASE];
  spi_flash_fill_in_address(sfd, &sfd->tmp_buf[1], addr);
  sfd->sequence_buf[1].tx = &sfd->tmp_buf[0];
  sfd->sequence_buf[1].tx_len = 1 + sfd->flash_conf.addressing_bytes + 1;
  sfd->sequence_buf[1].rx = 0;
  sfd->sequence_buf[1].rx_len = 0;
  sfd->sequence_buf[1].cs_release = 0;

  int res = spi_flash_exec(sfd, &sfd->sequence_buf[0], 2);

  if (res == SPI_OK) {
    sfd->addr += len;
    sfd->count -= len;
  }

  return res;
}

int SPI_FLASH_erase(spi_flash_dev *sfd, spi_flash_callback cb, u32_t addr, u32_t size) {
  if (!sfd->open) {
    return SPI_FLASH_ERR_CLOSED;
  }
  if (addr > sfd->flash_conf.size_total || (addr+size) > sfd->flash_conf.size_total) {
    return SPI_FLASH_ERR_ADDRESS;
  }
  if (sfd->busy) {
    return SPI_FLASH_ERR_BUSY;
  }
  sfd->busy = TRUE;
  sfd->state = SPI_FLASH_STATE_ERASE_SEQ;

  // adjust addr
  {
    u32_t blocksize = (sfd->flash_conf.size_sector_erase_min-1);
    u32_t adjust = addr & blocksize;
    addr -= adjust;
    size += adjust;

    // adjust len
    adjust = blocksize + 1 - ((addr + size) & blocksize);
    if (adjust < blocksize + 1) {
      size += adjust;
    }

    ASSERT((addr & blocksize) == 0);
    ASSERT((size & blocksize) == 0);
  }

  sfd->addr = addr;
  sfd->count = size;
  sfd->spi_flash_callback = cb;

  return spi_flash_send_erase_sequence(sfd);
}

int SPI_FLASH_protect(spi_flash_dev *sfd, spi_flash_callback cb) {
  if (!sfd->open) {
    return SPI_FLASH_ERR_CLOSED;
  }
  if (sfd->busy) {
    return SPI_FLASH_ERR_BUSY;
  }
  sfd->busy = TRUE;
  sfd->state = SPI_FLASH_STATE_WRSR_SEQ;
  sfd->spi_flash_callback = cb;

  return spi_flash_closeprotectun(sfd, FALSE);
}

int SPI_FLASH_unprotect(spi_flash_dev *sfd, spi_flash_callback cb) {
  if (!sfd->open) {
    return SPI_FLASH_ERR_CLOSED;
  }
  if (sfd->busy) {
    return SPI_FLASH_ERR_BUSY;
  }
  sfd->busy = TRUE;
  sfd->state = SPI_FLASH_STATE_WRSR_SEQ;
  sfd->spi_flash_callback = cb;

  return spi_flash_closeprotectun(sfd, TRUE);
}

int SPI_FLASH_mass_erase(spi_flash_dev *sfd, spi_flash_callback cb) {
  if (!sfd->open) {
    return SPI_FLASH_ERR_CLOSED;
  }
  if (sfd->busy) {
    return SPI_FLASH_ERR_BUSY;
  }
  sfd->busy = TRUE;
  sfd->state = SPI_FLASH_STATE_MASS_ERASE;
  sfd->spi_flash_callback = cb;

  spi_flash_set_first_seq_to_wren(sfd);

  sfd->sequence_buf[1].tx = (u8_t*)&sfd->flash_conf.cmd_defs[SPI_FLASH_CMD_MASS_ERASE];
  sfd->sequence_buf[1].tx_len = 1;
  sfd->sequence_buf[1].rx = 0;
  sfd->sequence_buf[1].rx_len = 0;
  sfd->sequence_buf[1].cs_release = 0;

  return spi_flash_exec(sfd, &sfd->sequence_buf[0], 2);
}


int SPI_FLASH_init(spi_flash_dev *sfd, spi_flash_dev_conf *flash_conf, u16_t spi_conf,
    spi_bus *bus, hw_io_port cs_port, hw_io_pin cs_pin) {
  memset(sfd, 0, sizeof(spi_flash_dev));
  memcpy(&sfd->flash_conf, flash_conf, sizeof(spi_flash_dev_conf));
  //SPI_DEV_init(&sfd->dev, spi_conf, bus, cs_port, cs_pin, 0);
  SPI_DEV_init(&sfd->dev, spi_conf, bus, cs_port, cs_pin, SPI_CONF_IRQ_DRIVEN);
  SPI_DEV_set_callback(&sfd->dev, spi_flash_callback_spi_result);
  sfd->task = TASK_create(spi_flash_task_f, TASK_STATIC);
  sfd->state = SPI_FLASH_STATE_CLOSED;
  sfd->open = FALSE;
  return SPI_OK;
}

char SPI_FLASH_is_busy(spi_flash_dev *sfd) {
  return SPI_DEV_is_busy(&sfd->dev) | sfd->busy;
}

void SPI_FLASH_dump(spi_flash_dev *sfd) {
  print("SPI FLASH DEV\n-------------\n");
  print("  bus busy:%i rx:%p rx_len:%i uarg:%08x uptr:%p\n",
        sfd->dev.bus->busy,
        sfd->dev.bus->rx_buf,
        sfd->dev.bus->rx_len,
        sfd->dev.bus->user_arg,
        sfd->dev.bus->user_p);
  print("  dev conf:%02x tx_len:%04x rx_len:%04x cs_rel:%i seq_len:%i\n",
        sfd->dev.configuration,
        sfd->dev.cur_seq.tx_len,
        sfd->dev.cur_seq.rx_len,
        sfd->dev.cur_seq.cs_release,
        sfd->dev.seq_len);
  print("  fla busy:%i state:%i ptr:%p addr:%08x count:%08x\n",
        sfd->busy,
        sfd->state,
        sfd->ptr,
        sfd->addr,
        sfd->count);
}
#endif

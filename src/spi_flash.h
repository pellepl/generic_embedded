/*
 * spi_flash.h
 *
 *  Created on: Sep 23, 2012
 *      Author: petera
 */

#ifndef SPI_FLASH_H_
#define SPI_FLASH_H_

#include "system.h"
#include "spi_dev.h"
#include "taskq.h"

#define SPI_FLASH_ERR_BUSY            -2100
#define SPI_FLASH_ERR_OPENED          -2101
#define SPI_FLASH_ERR_CLOSED          -2102
#define SPI_FLASH_ERR_UNDEFINED_STATE -2103
#define SPI_FLASH_ERR_INVALID_ID      -2104
#define SPI_FLASH_ERR_ADDRESS         -2105

enum spi_flash_cmd_e {
  SPI_FLASH_CMD_READ_ID = 0,
  SPI_FLASH_CMD_WRITE_ENABLE,
  SPI_FLASH_CMD_READ_SR,
  SPI_FLASH_CMD_WRITE_SR,
  SPI_FLASH_CMD_SR_PROTECT,
  SPI_FLASH_CMD_SR_UNPROTECT,
  SPI_FLASH_CMD_READ,
  SPI_FLASH_CMD_WRITE,
  SPI_FLASH_CMD_SECTOR_ERASE,
  SPI_FLASH_CMD_MASS_ERASE,
  SPI_FLASH_CMD_SIZE
};

enum spi_flash_state_e {
  SPI_FLASH_STATE_ERROR = -1,
  SPI_FLASH_STATE_CLOSED = 0,
  SPI_FLASH_STATE_OPENED = 1,

  SPI_FLASH_STATE_CLOSING = 2,
  SPI_FLASH_STATE_OPENING_READ_ID = 3,

  SPI_FLASH_STATE_READ_BUSY = 4,

  SPI_FLASH_STATE_READ = 5,

  SPI_FLASH_STATE_WRITE_SEQ = 6,
  SPI_FLASH_STATE_WRITE_WAIT = 7,

  SPI_FLASH_STATE_ERASE_SEQ = 8,
  SPI_FLASH_STATE_ERASE_WAIT = 9,

  SPI_FLASH_STATE_WRSR_SEQ = 10,
  SPI_FLASH_STATE_WRSR_WAIT = 11,

  SPI_FLASH_STATE_MASS_ERASE = 12

};

typedef struct {
  u32_t flash_id;
  u32_t size_total;
  u32_t size_page_write_max;
  u32_t size_sector_erase_min;
  u16_t time_sr_write_ms;
  u16_t time_page_write_ms;
  u16_t time_sector_erase_ms;
  u16_t time_mass_erase_ms;
  const u8_t *cmd_defs;

  u8_t busy_poll_divisor : 4;     /* 0-15, 0 == no sr busy check polling */
  u8_t busy_poll_min_time : 4;    /* 0-15, minimum poll time in ms + 1 */
  u8_t busy_sr_bit : 3;           /* 0-7, bit in sr indicating busy */
  u8_t addressing_bytes : 2;      /* 0-3, bytes of addressing + 1 */
  u8_t addressing_endianess : 1;  /* 0 = big endian, 1 = little endian */
} spi_flash_dev_conf;

typedef struct spi_flash_dev_s {
  spi_dev dev;
  enum spi_flash_state_e state;
  bool busy;
  bool open;
  spi_flash_dev_conf flash_conf;
  task *task;
  task_timer timer;
  void (*spi_flash_callback)(struct spi_flash_dev_s *dev, int result);
  bool busy_poll;
  u8_t poll_count;

  // some generic data holders
  spi_dev_sequence sequence_buf[3];
  u8_t tmp_buf[6];
  void* ptr;
  u32_t count;
  u32_t addr;
  void* sr_dst;
  u8_t sr_tmp;
} spi_flash_dev;

typedef void (*spi_flash_callback)(spi_flash_dev *dev, int result);

int SPI_FLASH_init(spi_flash_dev *sfd, spi_flash_dev_conf *flash_conf, u16_t spi_conf,
    spi_bus *bus, hw_io_port cs_port, hw_io_pin cs_pin);
int SPI_FLASH_open(spi_flash_dev *sfd, spi_flash_callback cb);
int SPI_FLASH_close(spi_flash_dev *sfd, spi_flash_callback cb);
int SPI_FLASH_close_force(spi_flash_dev *sfd);
int SPI_FLASH_read_busy(spi_flash_dev *sfd, spi_flash_callback cb, u8_t *res);
int SPI_FLASH_read(spi_flash_dev *sfd, spi_flash_callback cb, u32_t addr, u16_t size, u8_t *dest);
int SPI_FLASH_write(spi_flash_dev *sfd, spi_flash_callback cb, u32_t addr, u16_t size, u8_t *src);
int SPI_FLASH_erase(spi_flash_dev *sfd, spi_flash_callback cb, u32_t addr, u32_t size);
int SPI_FLASH_mass_erase(spi_flash_dev *sfd, spi_flash_callback cb);
int SPI_FLASH_protect(spi_flash_dev *sfd, spi_flash_callback cb);
int SPI_FLASH_unprotect(spi_flash_dev *sfd, spi_flash_callback cb);
char SPI_FLASH_is_busy(spi_flash_dev *sfd);
void SPI_FLASH_dump(spi_flash_dev *sfd);

#endif /* SPI_FLASH_H_ */

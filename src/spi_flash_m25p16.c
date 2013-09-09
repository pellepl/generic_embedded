/*
 * spi_flash_m25p16.c
 *
 *  Created on: Sep 22, 2012
 *      Author: petera
 */

#include "spi_flash_m25p16.h"
#ifdef CONFIG_SPI

static const u8_t CMD_DEFS[SPI_FLASH_CMD_SIZE] = {
    READ_ID,
    WRITE_ENABLE,
    READ_SR,
    WRITE_SR,
    0xff, /* sr setting for protecting */
    0x00, /* sr setting for unprotecting */
    READ,
    WRITE,
    ERASE,
    MASS_ERASE
};

spi_flash_dev __m25p16_dev;

void SPI_FLASH_M25P16_app_init() {
  SPI_FLASH_M25P16_init(
      &__m25p16_dev,
      FLASH_SPI_CONFIG,
      _SPI_BUS(FLASH_SPI_BUS),
      SPI_FLASH_GPIO_PORT,
      SPI_FLASH_GPIO_PIN);
}


void SPI_FLASH_M25P16_init(spi_flash_dev *sfd, u16_t spi_conf, spi_bus *bus,
    hw_io_port cs_port, hw_io_pin cs_pin) {

  spi_flash_dev_conf conf;
  conf.addressing_bytes = 2;
  conf.addressing_endianess = 0;
  conf.busy_poll_divisor = 4;
  conf.busy_poll_min_time = 4;
  conf.busy_sr_bit = FLASH_M25P16_BUSY_SR_BIT;
  conf.flash_id = FLASH_M25P16_ID;
  conf.size_page_write_max = FLASH_M25P16_SIZE_PAGE_WRITE;
  conf.size_sector_erase_min = FLASH_M25P16_SIZE_SECTOR_ERASE;
  conf.size_total = FLASH_M25P16_SIZE_TOTAL;
  conf.time_mass_erase_ms = FLASH_M25P16_TIME_MASS_ERASE;
  conf.time_page_write_ms = FLASH_M25P16_TIME_PAGE_WRITE;
  conf.time_sector_erase_ms = FLASH_M25P16_TIME_SECTOR_ERASE;
  conf.time_sr_write_ms = FLASH_M25P16_TIME_SR_WRITE;
  conf.cmd_defs = CMD_DEFS;
  SPI_FLASH_init(sfd, &conf, spi_conf, bus, cs_port, cs_pin);
}

#endif

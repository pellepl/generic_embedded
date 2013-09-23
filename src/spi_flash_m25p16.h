/*
 * spi_flash_m25p16.h
 *
 *  Created on: Sep 22, 2012
 *      Author: petera
 */

#ifndef SPI_FLASH_M25P16_H_
#define SPI_FLASH_M25P16_H_

#include "spi_flash.h"

enum {
  WRITE_ENABLE = 0x06,
  WRITE_DISABLE = 0x04,
  READ_ID = 0x9f,
  READ_SR = 0x05,
  WRITE_SR = 0x01,
  READ = 0x03,
  WRITE = 0x02,
  ERASE = 0xd8,
  MASS_ERASE = 0xc7
} spi_flash_m25p16_cmd;

#ifndef FLASH_SPI_CONFIG
#define FLASH_SPI_CONFIG                SPIDEV_CONFIG_CPHA_2E | SPIDEV_CONFIG_CPOL_HI | SPIDEV_CONFIG_FBIT_MSB | SPIDEV_CONFIG_SPEED_HIGHEST
#endif

#ifndef FLASH_SPI_BUS
#define FLASH_SPI_BUS                   (0)
#endif

#define FLASH_M25P16_ID                 0x00202015

#define FLASH_M25P16_SIZE_TOTAL         (1024*1024*2)

#define FLASH_M25P16_SIZE_PAGE_WRITE    256

#define FLASH_M25P16_SIZE_SECTOR_ERASE  65536

#define FLASH_M25P16_TIME_SR_WRITE      15

#define FLASH_M25P16_TIME_PAGE_WRITE    5

#define FLASH_M25P16_TIME_SECTOR_ERASE  3000

#define FLASH_M25P16_TIME_MASS_ERASE    40000

#define FLASH_M25P16_BUSY_SR_BIT        0

void SPI_FLASH_M25P16_init(spi_flash_dev *sfd, u16_t spi_conf, spi_bus *bus,
    hw_io_port cs_port, hw_io_pin cs_pin);

void SPI_FLASH_M25P16_app_init();

extern spi_flash_dev __m25p16_dev;

#define SPI_FLASH (&__m25p16_dev)

#endif /* SPI_FLASH_M25P16_H_ */

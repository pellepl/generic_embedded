/*
 * spi_flash_os.h
 *
 *  Created on: May 23, 2013
 *      Author: petera
 */

#ifndef SPI_FLASH_OS_H_
#define SPI_FLASH_OS_H_

#include "system.h"

s32_t SFOS_erase(u32_t addr, u32_t size);
s32_t SFOS_read(u32_t addr, u32_t size, u8_t *dst);
s32_t SFOS_write(u32_t addr, u32_t size, u8_t *src);
void SFOS_init();


#endif /* SPI_FLASH_OS_H_ */

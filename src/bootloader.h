/*
 * bootloader.h
 *
 *  Created on: Apr 6, 2013
 *      Author: petera
 */

#ifndef BOOTLOADER_H_
#define BOOTLOADER_H_

#include "bl_exec.h"

//#define DBG_BL

#define BOOTLOADER_TEXT __attribute__ (( section(".bootloader_text"), used, long_call ))
#define BOOTLOADER_DATA __attribute__ (( section(".bootloader_data"), used ))

typedef struct {
  bootloader_operation operation;
  u32_t suboperation;
  USART_TypeDef *uart_hw;
  SPI_TypeDef *spi_hw;
} bootdata;

#define FW_FLASH_BASE (FLASH_BASE)

typedef enum {
  FLASH_OK = 0,
  FLASH_ERR_BUSY,
  FLASH_ERR_WRITE_PROTECTED,
  FLASH_ERR_TIMEOUT,
  FLASH_ERR_OTHER
} FLASH_res;

extern bootdata boot;

BOOTLOADER_TEXT void b_put(char c);
BOOTLOADER_TEXT void b_putstr(const char *s);
BOOTLOADER_TEXT void b_puthex8(u8_t x);
BOOTLOADER_TEXT void b_puthex16(u16_t x);
BOOTLOADER_TEXT void b_puthex32(u32_t x);
BOOTLOADER_TEXT void b_puthex_buf(u8_t *d, u32_t len);
BOOTLOADER_TEXT void b_putint(s32_t x);

BOOTLOADER_TEXT void b_spif_read(u32_t addr, u8_t *b, u16_t len);
BOOTLOADER_TEXT void b_spif_write(u32_t addr, u8_t *b, u16_t len);

BOOTLOADER_TEXT void b_flash_open();
BOOTLOADER_TEXT void b_flash_close();
BOOTLOADER_TEXT FLASH_res b_flash_erase(u32_t addr);
BOOTLOADER_TEXT FLASH_res b_flash_write_hword(u32_t addr, u16_t data);
BOOTLOADER_TEXT FLASH_res b_flash_unprotect();
BOOTLOADER_TEXT FLASH_res b_flash_protect();

BOOTLOADER_TEXT void bootloader_init();

#ifdef DBG_BL
#define BLDBG(s) b_putstr((s))
#else
#define BLDBG(s)
#endif

#endif /* BOOTLOADER_H_ */

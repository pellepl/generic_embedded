/*
 * bootloader.h
 *
 * The address given to the firmware binary is expected to point to
 * a fw_upgrade_info struct, containing a magic number, firmware length,
 * 16 bit CCITT crc of firmware data, and the firmware data file name (up to 64 bytes).
 * Directly after this struct the firmware binary is supposed to follow.
 *
 * The bootloader uses the macro BL_IMG_READ to read this struct and the binary. This
 * define can reading a spi flash, another flash bank, ethernet, or whatever else that can
 * be accessed by means of address and length.
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
  void *uart_hw;
  void *media_hw;
} bootdata;

#define FW_FLASH_BASE (FLASH_BASE)

typedef enum {
  FLASH_OK = 0,
  FLASH_ERR_BUSY,
  FLASH_ERR_WRITE_PROTECTED,
  FLASH_ERR_TIMEOUT,
  FLASH_ERR_OTHER
} FLASH_res;

extern bootdata __boot;

BOOTLOADER_TEXT void bootloader_init();

// uart hal
BOOTLOADER_TEXT void b_put(char c);
BOOTLOADER_TEXT void b_putstr(const char *s);
BOOTLOADER_TEXT void b_puthex8(u8_t x);
BOOTLOADER_TEXT void b_puthex16(u16_t x);
BOOTLOADER_TEXT void b_puthex32(u32_t x);
BOOTLOADER_TEXT void b_puthex_buf(u8_t *d, u32_t len);
BOOTLOADER_TEXT void b_putint(s32_t x);

// flash hal
BOOTLOADER_TEXT void b_flash_open();
BOOTLOADER_TEXT void b_flash_close();
BOOTLOADER_TEXT void b_flash_get_sector(u32_t phys_addr, u32_t *sect_addr,
    u32_t *sect_len);
BOOTLOADER_TEXT FLASH_res b_flash_erase(u32_t sect_addr);
BOOTLOADER_TEXT FLASH_res b_flash_write_hword(u32_t phys_addr, u16_t data);
BOOTLOADER_TEXT FLASH_res b_flash_unprotect();
BOOTLOADER_TEXT FLASH_res b_flash_protect();

#ifdef CONFIG_SPI
// spi hal
BOOTLOADER_TEXT void b_spif_read(u32_t addr, u8_t *b, u16_t len);
BOOTLOADER_TEXT void b_spif_write(u32_t addr, u8_t *b, u16_t len);
#endif

// define bootloader firmware image location read and write
#ifndef BL_IMG_READ
#if defined(CONFIG_SPI)
#define BL_IMG_READ(a, b, l) b_spif_read((a), (b), (l))
#define BL_IMG_WRITE(a, b, l) b_spif_write((a), (b), (l))
#else
#define BL_IMG_READ(a, b, l) do { \
    u32_t i; \
    u8_t *d = (u8_t *)(a); \
    for (i = 0; i < (l); i++) { \
      (b)[i] = d[i]; \
    }\
  } while(0)
#define BL_IMG_WRITE(a, b, l) (void)(a);(void)(b);void(l)
#endif
#endif

// bootloader debug output
#ifdef DBG_BL
#define BLDBG(s) b_putstr((s))
#else
#define BLDBG(s)
#endif

#endif /* BOOTLOADER_H_ */

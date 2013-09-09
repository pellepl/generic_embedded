#include "bootloader.h"
#include "bl_exec.h"
#include "linker_symaccess.h"

#define SHMEM ((shmem *)SHARED_MEMORY_ADDRESS)

BOOTLOADER_DATA bootdata boot;

BOOTLOADER_TEXT static bool _bootloader_check_spi_flash();
BOOTLOADER_TEXT static FLASH_res _bootloader_flash_erase();
BOOTLOADER_TEXT static FLASH_res _bootloader_flash_upgrade();

BOOTLOADER_TEXT static void _bootloader_reset(enum reboot_reason_e rr, u32_t subop) {
  //b_putstr(boot_msg_bye);
  b_putstr("**** Bootloader returning, bye\n\n");
  SHMEM->reboot_reason = rr;
  SHMEM->user[BOOTLOADER_SHMEM_SUBOPERATION_UIX] = subop;
  SHMEM_CALC_CHK(SHMEM, SHMEM->chk);
  NVIC_SystemReset();
  while(1);
}

BOOTLOADER_TEXT static void _bootloader_fail(char *err, int errcode) {
  b_putstr(err);
  b_putint(errcode);
  b_put('\n');
  _bootloader_reset(REBOOT_BOOTLOADER, 0);
}

BOOTLOADER_TEXT void bootloader_init() {
  boot.uart_hw = (USART_TypeDef*)(SHMEM->user[BOOTLOADER_SHMEM_UART_UIX]);
  boot.spi_hw = (SPI_TypeDef*)(SHMEM->user[BOOTLOADER_SHMEM_SPI_FLASH_UIX]);
  boot.operation = SHMEM->user[BOOTLOADER_SHMEM_OPERATION_UIX];
  boot.suboperation = SHMEM->user[BOOTLOADER_SHMEM_SUBOPERATION_UIX];
  b_putstr("**** Bootloader ram domain entered\n");

  b_putstr("  stack pointer: ");
  b_puthex32(__get_MSP());
  b_put('\n');

  b_putstr("  operation: ");
  b_putint(boot.operation);
  b_putstr(", subop: ");
  b_putint(boot.suboperation);
  b_put('\n');

  FLASH_res res;
  switch (boot.operation) {
  case BOOTLOADER_FLASH_FIRMWARE:
    switch (boot.suboperation) {
    case 0:
      // check spi flash contents
      b_putstr("  check spi flash image crc...\n");
      if (_bootloader_check_spi_flash()) {
        b_putstr("  OK\n");
      } else {
        _bootloader_reset(REBOOT_BOOTLOADER, 0);
      }

      b_flash_open();

      // unprotect
      b_putstr("  unprotecting flash...\n");
      res = b_flash_unprotect();
      if (res == FLASH_OK) {
        b_putstr("  OK\n");
        _bootloader_reset(REBOOT_EXEC_BOOTLOADER, 1);
      } else {
        _bootloader_fail("  flash operation failed: ", res);
      }
      break;
    case 1:
      b_flash_open();

      // erase
      b_putstr("  erase flash...\n");
      res = _bootloader_flash_erase();
      if (res == FLASH_OK) {
        b_putstr("  OK\n");
      } else {
        _bootloader_fail("  flash operation failed: ", res);
      }

      // flash
      b_putstr("  upgrading flash...\n");
      res = _bootloader_flash_upgrade();
      if (res == FLASH_OK) {
        b_putstr("  OK\n");
      } else {
        _bootloader_fail("  flash operation failed: ", 0);
      }

      // protect
      b_putstr("  protecting flash...\n");
      res = b_flash_protect();
      if (res == FLASH_OK) {
        b_putstr("  OK\n");
        b_putstr("  flashing procedure complete!\n");
        _bootloader_reset(REBOOT_BOOTLOADER, 0);
      } else {
        _bootloader_fail("  flash operation failed: ", res);
      }
      break;
    }
    break;
  case BOOTLOADER_EXECUTE:
  default:
    _bootloader_reset(REBOOT_BOOTLOADER, 0);
    break;
  }
}

BOOTLOADER_TEXT static FLASH_res _bootloader_flash_erase() {
  fw_upgrade_info fui;
  u32_t spi_addr = SHMEM->user[BOOTLOADER_SHMEM_SPIF_FW_ADDR_UIX];
  u32_t flash_addr = FW_FLASH_BASE;
  FLASH_res res = FLASH_OK;

  b_spif_read(spi_addr, (u8_t*)&fui, sizeof(fui));

  u32_t pages = (fui.len / FLASH_PAGE_SIZE) + ((fui.len % FLASH_PAGE_SIZE) == 0 ? 0:1);

  b_putstr("  erasing flash @ ");
  b_puthex32(flash_addr);
  b_putstr(", ");
  b_putint(fui.len);
  b_putstr(" bytes, ");
  b_putint(pages);
  b_putstr(" pages of size ");
  b_putint(FLASH_PAGE_SIZE);
  b_putstr(" => ");
  b_putint(pages*FLASH_PAGE_SIZE);
  b_putstr(" bytes\n");

  u32_t page = 0;

  while (res == FLASH_OK && page < pages) {
    b_putstr("  erasing page ");
    b_putint(page);
    b_putstr(" @ ");
    b_puthex32(flash_addr);
    b_putstr(".. ");
    res = b_flash_erase(flash_addr);
    if (res == FLASH_OK) {
      b_putstr(" OK\n");
    } else {
      b_putstr(" fail: ");
      b_putint(res);
      b_put('\n');
    }
    flash_addr += FLASH_PAGE_SIZE;
    page++;
  }

  return res;
}

BOOTLOADER_TEXT static FLASH_res _bootloader_flash_upgrade() {
  fw_upgrade_info fui;
  u16_t buf[256/sizeof(u16_t)];
  u32_t spi_addr = SHMEM->user[BOOTLOADER_SHMEM_SPIF_FW_ADDR_UIX];
  u32_t flash_addr = FW_FLASH_BASE;
  u32_t spi_ix = 0;
  FLASH_res res = FLASH_OK;

  b_spif_read(spi_addr, (u8_t*)&fui, sizeof(fui));

  spi_addr += sizeof(fui);
  b_putstr("  flashing and verifying spi flash image contents @ ");
  b_puthex32(spi_addr);
  b_putstr(" to iflash @ ");
  b_puthex32(flash_addr);
  b_putstr(", ");
  b_putint(fui.len);
  b_putstr(" bytes, file: ");
  b_putstr((char*)fui.fname);
  b_put('\n');

  while (res == FLASH_OK && spi_ix < fui.len) {
    u32_t b_ix;
    u16_t len = MIN(sizeof(buf), fui.len - spi_ix);
    b_spif_read(spi_addr + spi_ix, (u8_t*)&buf[0], len);

    for (b_ix = 0; res == FLASH_OK && b_ix < len; b_ix += sizeof(u16_t)) {
      u16_t eword = *((u16_t*)flash_addr);
      if (eword != 0xffff) {
        b_putstr("  ERR address ");
        b_puthex32(flash_addr);
        b_putstr(" not erased\n");
        res = FLASH_ERR_OTHER;
        break;
      }
      if ((flash_addr & 0x3ff) == 0) {
        b_putstr("  upgrading flash ");
        b_putint((100 * (flash_addr - FW_FLASH_BASE)) / fui.len);
        b_putstr("% @ ");
        b_puthex32(flash_addr);
        b_put('\n');
      }
      u16_t wword = buf[b_ix/sizeof(u16_t)];
      res = b_flash_write_hword(flash_addr, wword);
      u16_t rword = *((u16_t*)flash_addr);
      if (rword != wword) {
        b_putstr("  ERR DIFF want ");
        b_puthex16(wword);
        b_putstr(" got ");
        b_puthex16(rword);
        b_putstr(" @ ");
        b_puthex32(flash_addr);
        b_put('\n');
        res = FLASH_ERR_OTHER;
        break;
      }
      flash_addr += sizeof(u16_t);
    }

    spi_ix += len;
  }

  b_putstr("  dumping first 1024 bytes...\n");
  u32_t a;
  for (a = FW_FLASH_BASE; a < FW_FLASH_BASE + 1024; a += 32) {
    b_puthex32(a);
    b_putstr(": ");
    b_puthex_buf((u8_t*)a,32);
    b_put('\n');
  }

  if (res == FLASH_OK) {
    b_putstr("  upgrade finished\n");
  }
  return res;

}

BOOTLOADER_TEXT static u16_t _bootloader_crc_ccitt_16(u16_t crc, u8_t data) {
  crc  = (u8_t)(crc >> 8) | (crc << 8);
  crc ^= data;
  crc ^= (u8_t)(crc & 0xff) >> 4;
  crc ^= (crc << 8) << 4;
  crc ^= ((crc & 0xff) << 4) << 1;
  return crc;
}


BOOTLOADER_TEXT static bool _bootloader_check_spi_flash() {
  fw_upgrade_info fui;
  u32_t addr = SHMEM->user[BOOTLOADER_SHMEM_SPIF_FW_ADDR_UIX];
  b_spif_read(addr, (u8_t*)&fui, sizeof(fui));
  if (fui.magic != FW_MAGIC) {
    b_putstr("  bad fw magic: ");
    b_puthex32(fui.magic);
    b_put('\n');
    return FALSE;
  }
  b_putstr("  found fw file:");
  b_putstr((char*)&fui.fname[0]);
  b_putstr(", len:");
  b_putint(fui.len);
  b_put('\n');

  // calc crc
  u16_t crc = 0xffff;
  const u32_t start = SHMEM->user[BOOTLOADER_SHMEM_SPIF_FW_ADDR_UIX] + sizeof(fw_upgrade_info);
  for (addr = start; addr < start + fui.len; addr++) {
    u8_t c;
    b_spif_read(addr, (u8_t*)&c, 1);
    crc = _bootloader_crc_ccitt_16(crc, c);
  }

  if (crc != fui.crc) {
    b_putstr("  fw file bad crc:");
    b_puthex16(crc);
    b_put('/');
    b_puthex16(fui.crc);
    b_put('\n');
    return FALSE;
  }
  return TRUE;
}

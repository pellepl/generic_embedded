#include "system.h"
#include "bootloader.h"
#include "linker_symaccess.h"
#include "stm32f4xx_flash.h"

// Internal flash helpers

/* Base address of the Flash sectors */
#define ADDR_FLASH_SECTOR_0     ((uint32_t)0x08000000) /* Base @ of Sector 0, 16 Kbytes */
#define ADDR_FLASH_SECTOR_1     ((uint32_t)0x08004000) /* Base @ of Sector 1, 16 Kbytes */
#define ADDR_FLASH_SECTOR_2     ((uint32_t)0x08008000) /* Base @ of Sector 2, 16 Kbytes */
#define ADDR_FLASH_SECTOR_3     ((uint32_t)0x0800C000) /* Base @ of Sector 3, 16 Kbytes */
#define ADDR_FLASH_SECTOR_4     ((uint32_t)0x08010000) /* Base @ of Sector 4, 64 Kbytes */
#define ADDR_FLASH_SECTOR_5     ((uint32_t)0x08020000) /* Base @ of Sector 5, 128 Kbytes */
#define ADDR_FLASH_SECTOR_6     ((uint32_t)0x08040000) /* Base @ of Sector 6, 128 Kbytes */
#define ADDR_FLASH_SECTOR_7     ((uint32_t)0x08060000) /* Base @ of Sector 7, 128 Kbytes */
#define ADDR_FLASH_SECTOR_8     ((uint32_t)0x08080000) /* Base @ of Sector 8, 128 Kbytes */
#define ADDR_FLASH_SECTOR_9     ((uint32_t)0x080A0000) /* Base @ of Sector 9, 128 Kbytes */
#define ADDR_FLASH_SECTOR_10    ((uint32_t)0x080C0000) /* Base @ of Sector 10, 128 Kbytes */
#define ADDR_FLASH_SECTOR_11    ((uint32_t)0x080E0000) /* Base @ of Sector 11, 128 Kbytes */

#define SECTOR_MASK               ((uint32_t)0xFFFFFF07)

/* Delay definition */
#define FLASH_TIMEOUT            ((uint32_t)0x00B00000)

BOOTLOADER_TEXT static FLASH_res _b_flash_status() {
  FLASH_res res = FLASH_OK;

  if ((FLASH->SR & FLASH_FLAG_BSY) == FLASH_FLAG_BSY) {
    res = FLASH_ERR_BUSY;
  } else if ((FLASH->SR & FLASH_FLAG_WRPERR) != 0) {
    BLDBG("FLASH_ERR_WRP\n");
    res = FLASH_ERROR_WRP;
  } else if ((FLASH->SR & (uint32_t)0xEF) != (uint32_t)0x00 ||
      (FLASH->SR & FLASH_FLAG_OPERR) != (uint32_t)0x00) {
    BLDBG("FLASH_ERR_OTHER\n");
    res = FLASH_ERR_OTHER;
  } else {
    res = FLASH_OK;
  }

  return res;
}

BOOTLOADER_TEXT static FLASH_res _b_flash_wait(u32_t timeout) {
  FLASH_res res;

  while (((res = _b_flash_status()) == FLASH_ERR_BUSY) && timeout) {
    timeout--;
  }

  if (timeout == 0) {
    BLDBG("FLASH_ERR_TIMEOUT\n");
    res = FLASH_ERR_TIMEOUT;
  }

  return res;
}

BOOTLOADER_TEXT void b_flash_get_sector(u32_t phys_addr, u32_t *sect_addr, u32_t *sect_len) {
  if((phys_addr < ADDR_FLASH_SECTOR_1) && (phys_addr >= ADDR_FLASH_SECTOR_0))
  {
    *sect_addr = FLASH_Sector_0;
    *sect_len = 16*1024;
  }
  else if((phys_addr < ADDR_FLASH_SECTOR_2) && (phys_addr >= ADDR_FLASH_SECTOR_1))
  {
    *sect_addr = FLASH_Sector_1;
    *sect_len = 16*1024;
  }
  else if((phys_addr < ADDR_FLASH_SECTOR_3) && (phys_addr >= ADDR_FLASH_SECTOR_2))
  {
    *sect_addr = FLASH_Sector_2;
    *sect_len = 16*1024;
  }
  else if((phys_addr < ADDR_FLASH_SECTOR_4) && (phys_addr >= ADDR_FLASH_SECTOR_3))
  {
    *sect_addr = FLASH_Sector_3;
    *sect_len = 16*1024;
  }
  else if((phys_addr < ADDR_FLASH_SECTOR_5) && (phys_addr >= ADDR_FLASH_SECTOR_4))
  {
    *sect_addr = FLASH_Sector_4;
    *sect_len = 64*1024;
  }
  else if((phys_addr < ADDR_FLASH_SECTOR_6) && (phys_addr >= ADDR_FLASH_SECTOR_5))
  {
    *sect_addr = FLASH_Sector_5;
    *sect_len = 128*1024;
  }
  else if((phys_addr < ADDR_FLASH_SECTOR_7) && (phys_addr >= ADDR_FLASH_SECTOR_6))
  {
    *sect_addr = FLASH_Sector_6;
    *sect_len = 128*1024;
  }
  else if((phys_addr < ADDR_FLASH_SECTOR_8) && (phys_addr >= ADDR_FLASH_SECTOR_7))
  {
    *sect_addr = FLASH_Sector_7;
    *sect_len = 128*1024;
  }
  else if((phys_addr < ADDR_FLASH_SECTOR_9) && (phys_addr >= ADDR_FLASH_SECTOR_8))
  {
    *sect_addr = FLASH_Sector_8;
    *sect_len = 128*1024;
  }
  else if((phys_addr < ADDR_FLASH_SECTOR_10) && (phys_addr >= ADDR_FLASH_SECTOR_9))
  {
    *sect_addr = FLASH_Sector_9;
    *sect_len = 128*1024;
  }
  else if((phys_addr < ADDR_FLASH_SECTOR_11) && (phys_addr >= ADDR_FLASH_SECTOR_10))
  {
    *sect_addr = FLASH_Sector_10;
    *sect_len = 128*1024;
  }
  else /*(phys_addr < FLASH_END_ADDR) && (phys_addr >= ADDR_FLASH_SECTOR_11))*/
  {
    *sect_addr = FLASH_Sector_11;
    *sect_len = 128*1024;
  }
}

BOOTLOADER_TEXT void b_flash_open() {
  // unlock flash block
  BLDBG("FLASH: open\n");

  // FLASH_UnlockBank1()
  FLASH->KEYR = FLASH_KEY1;
  FLASH->KEYR = FLASH_KEY2;
  // clear flags
  FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
                  FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR|FLASH_FLAG_PGSERR);
}

BOOTLOADER_TEXT void b_flash_close() {
  //FLASH_LockBank1();
  BLDBG("FLASH: close\n");

  FLASH->CR |= FLASH_CR_LOCK;
}

BOOTLOADER_TEXT FLASH_res b_flash_erase(u32_t sect_addr) {
  BLDBG("FLASH: erase\n");

  FLASH_res status = _b_flash_wait(FLASH_TIMEOUT);

  if (status == FLASH_OK) {
    // assume VoltageRange_3, 2.7V to 3.6V
    u32_t tmp_psize = FLASH_PSIZE_WORD;

    FLASH->CR &= CR_PSIZE_MASK;
    FLASH->CR |= tmp_psize;
    FLASH->CR &= SECTOR_MASK;
    FLASH->CR |= FLASH_CR_SER | sect_addr;
    FLASH->CR |= FLASH_CR_STRT;

    status = _b_flash_wait(FLASH_TIMEOUT );

    // if the erase operation is completed, disable the SER Bit
    FLASH->CR &= (~FLASH_CR_SER);
    FLASH->CR &= SECTOR_MASK;
  }
  return status;
}

BOOTLOADER_TEXT FLASH_res b_flash_write_hword(u32_t addr, u16_t data) {
  //FLASH_ProgramHalfWord(Address, Data);
  BLDBG("FLASH: write\n");
  FLASH_res res = _b_flash_wait(FLASH_TIMEOUT);

  if (res == FLASH_OK) {
    // if the previous operation is completed, proceed to program the new first
    // half word
    FLASH->CR &= CR_PSIZE_MASK;
    FLASH->CR |= FLASH_PSIZE_HALF_WORD;
    FLASH->CR |= FLASH_CR_PG;

    *(__IO uint16_t*) addr = (uint16_t) data;
    res = _b_flash_wait(FLASH_TIMEOUT);
  }
  // Disable the PG Bit
  FLASH->CR &= (~FLASH_CR_PG);

  return res;
}

BOOTLOADER_TEXT FLASH_res b_flash_set_protection(bool enable) {
  // Authorizes the Option Byte register programming
  FLASH->OPTKEYR = FLASH_OPT_KEY1;
  FLASH->OPTKEYR = FLASH_OPT_KEY2;

  FLASH_res res = _b_flash_wait(FLASH_TIMEOUT);

  if (res == FLASH_OK) {
    if (enable) {
      // enable all write protection
      *(__IO uint16_t*)OPTCR_BYTE2_ADDRESS &= ~OB_WRP_Sector_All;

    } else {
      // disable all write protection
      *(__IO uint16_t*)OPTCR_BYTE2_ADDRESS |= (u16_t)OB_WRP_Sector_All;
    }

    // launch option byte loading
    *(__IO uint8_t *)OPTCR_BYTE0_ADDRESS |= FLASH_OPTCR_OPTSTRT;
    res = _b_flash_wait(FLASH_TIMEOUT);

    // Set the OPTLOCK Bit to lock the FLASH Option Byte Registers access
    FLASH->OPTCR |= FLASH_OPTCR_OPTLOCK;
  }

  return res;

}

BOOTLOADER_TEXT FLASH_res b_flash_unprotect() {
  FLASH_res res;
  BLDBG("FLASH: unprotect\n");

  res = b_flash_set_protection(FALSE);

  // system reset must now be performed to release protection
  return res;
}

BOOTLOADER_TEXT FLASH_res b_flash_protect() {
  FLASH_res res;
  BLDBG("FLASH: protect\n");

  res = b_flash_set_protection(TRUE);

  return res;
}

#include "system.h"
#include "bootloader.h"
#include "linker_symaccess.h"

BOOTLOADER_TEXT static u8_t b_spi_txrx(u8_t c) {
  while ((((SPI_TypeDef *)(__boot.media_hw))->SR & SPI_I2S_FLAG_TXE )== 0);
  ((SPI_TypeDef *)(__boot.media_hw))->DR = c;
  while ((((SPI_TypeDef *)(__boot.media_hw))->SR & SPI_I2S_FLAG_RXNE )== 0);
  return ((SPI_TypeDef *)(__boot.media_hw))->DR;
}

BOOTLOADER_TEXT void b_spif_read(u32_t addr, u8_t *b, u16_t len) {
  GPIO_disable(SPI_FLASH_GPIO_PORT, SPI_FLASH_GPIO_PIN);

  b_spi_txrx(0x03); // read
  b_spi_txrx((addr >> 16) & 0xff);
  b_spi_txrx((addr >> 8) & 0xff);
  b_spi_txrx((addr >> 0) & 0xff);
  // get data
  while (len--) {
    *b++ = b_spi_txrx(0xff);
  }

  GPIO_enable(SPI_FLASH_GPIO_PORT, SPI_FLASH_GPIO_PIN);
}

BOOTLOADER_TEXT void b_spif_write(u32_t addr, u8_t *b, u16_t len) {
  GPIO_disable(SPI_FLASH_GPIO_PORT, SPI_FLASH_GPIO_PIN);
  b_spi_txrx(0x06); // write enable
  GPIO_enable(SPI_FLASH_GPIO_PORT, SPI_FLASH_GPIO_PIN);
  volatile int a = 0x100;
  while (a--)
    ;
  GPIO_disable(SPI_FLASH_GPIO_PORT, SPI_FLASH_GPIO_PIN);
  b_spi_txrx(0x02); // write
  b_spi_txrx((addr >> 16) & 0xff);
  b_spi_txrx((addr >> 8) & 0xff);
  b_spi_txrx((addr >> 0) & 0xff);
  // write data
  while (len--) {
    b_spi_txrx(*b++);
  }
  GPIO_enable(SPI_FLASH_GPIO_PORT, SPI_FLASH_GPIO_PIN);
}


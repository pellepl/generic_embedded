#include "system.h"
#include "bootloader.h"
#include "linker_symaccess.h"

// UART helpers

BOOTLOADER_TEXT void b_put(char c) {
  while ((((USART_TypeDef *)(__boot.uart_hw))->SR & USART_SR_TXE )== 0);
  ((USART_TypeDef *)(__boot.uart_hw))->DR = (c & (u16_t) 0x01FF);
}

BOOTLOADER_TEXT void b_putstr(const char *s) {
  char c;
  while ((c = *s++) != 0) {
    b_put(c);
  }
}

BOOTLOADER_TEXT void b_puthex_buf(u8_t *d, u32_t len) {
  while (len--) {
    b_puthex8(*d++);
  }
}

BOOTLOADER_TEXT void b_puthex32(u32_t x) {
  b_puthex16(x >> 16);
  b_puthex16(x);
}

BOOTLOADER_TEXT void b_puthex16(u16_t x) {
  b_puthex8(x >> 8);
  b_puthex8(x);
}

BOOTLOADER_TEXT void b_puthex8(u8_t x) {
  int i = 0;
  while (i++ < 2) {
    u8_t nibble = (x & (0xf0)) >> 4;
    if (nibble < 10) {
      b_put('0' + nibble);
    } else {
      b_put('a' + nibble - 10);
    }
    x <<= 4;
  }
}

BOOTLOADER_TEXT void b_putint(s32_t x) {

  if (x < 0) {
    x = -x;
    b_put('-');
  }
  if (x == 0) {
    b_put('0');
  }
  u8_t len = 0;
  char b[32];
  while (x > 0) {
    u8_t c = (x % 10);
    b[len] = ('0' + c);
    x /= 10;
    len++;
  }
  u8_t i = 0;
  while (i++ < len) {
    b_put(b[len-i]);
  }
}

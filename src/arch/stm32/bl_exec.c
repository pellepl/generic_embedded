#include "bl_exec.h"
#include "bootloader.h"
#include "miniutils.h"
#include "linker_symaccess.h"

#ifdef CONFIG_SPI
#include "spi_driver.h"
#include "spi_flash_m25p16.h"
#endif

static void _bootloader_setup_context() {
  // all interrupts off
  __disable_irq();
  // flush uart stdout
  UART_tx_flush(_UART(UARTSTDOUT));

  // entering deep waters..

  // copy text section from flash to ram
  memcpy(BOOTLOADER_TEXT_VMA_START, BOOTLOADER_TEXT_LMA_START, BOOTLOADER_TEXT_VMA_END - BOOTLOADER_TEXT_VMA_START);
  // copy data section from flash to ram
  memcpy(BOOTLOADER_DATA_VMA_START, BOOTLOADER_DATA_LMA_START, BOOTLOADER_DATA_VMA_END - BOOTLOADER_DATA_VMA_START);
  // reset stack pointer to end of ram (but save shmem)
  __set_MSP((u32_t)STACK_END);

  bootloader_init();
  SYS_reboot(REBOOT_USER);
  while(1);
}

static void _bootloader_setup_periph() {
#ifdef CONFIG_SPI
  print("  .. spi flash\n");
  GPIO_enable(SPI_FLASH_GPIO_PORT, SPI_FLASH_GPIO_PIN);

  SPI_config(_SPI_BUS(FLASH_SPI_BUS), FLASH_SPI_CONFIG);
  // save spi flash hw pointer
  SHMEM_get()->user[BOOTLOADER_SHMEM_MEDIA_UIX] = (u32_t)(_SPI_BUS(FLASH_SPI_BUS)->hw);
#endif
  // save stdout uart hw pointer
  SHMEM_get()->user[BOOTLOADER_SHMEM_UART_UIX] = (u32_t)(_UART(UARTSTDOUT)->hw);

  SHMEM_CALC_CHK(SHMEM_get(), SHMEM_get()->chk);
}

__attribute__ (( noreturn )) void bootloader_execute() {
  print("Configuring peripherals for bootloader\n");
  _bootloader_setup_periph();
  print("Bootloader initialization from kernel\n");
  print("Text section: %08x -> %08x, size %i bytes\n",
      BOOTLOADER_TEXT_LMA_START, BOOTLOADER_TEXT_VMA_START, (BOOTLOADER_TEXT_VMA_END - BOOTLOADER_TEXT_VMA_START));
  print("Data section: %08x -> %08x, size %i bytes\n",
      BOOTLOADER_DATA_LMA_START, BOOTLOADER_DATA_VMA_START, (BOOTLOADER_DATA_VMA_END - BOOTLOADER_DATA_VMA_START));
  print("SP will be set to %08x\n", STACK_END);
  print("Will jump to %08x after copy\n", bootloader_init);
  print("Kernel releasing command to bootloader in ram, bye... \n\n");
  _bootloader_setup_context();
  while(1);
}

void bootloader_update_fw() {
  SHMEM_get()->user[BOOTLOADER_SHMEM_OPERATION_UIX] = BOOTLOADER_FLASH_FIRMWARE;
  SHMEM_get()->user[BOOTLOADER_SHMEM_SUBOPERATION_UIX] = 0;
  SHMEM_get()->user[BOOTLOADER_SHMEM_SPIF_FW_ADDR_UIX] = FIRMWARE_SPIF_ADDRESS_META;
}

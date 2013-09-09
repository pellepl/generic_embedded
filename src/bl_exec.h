/*
 * bootloader_exec.h
 *
 *  Created on: Apr 7, 2013
 *      Author: petera
 */

#include "system.h"

#ifndef BOOTLOADER_EXEC_H_
#define BOOTLOADER_EXEC_H_

typedef struct  __attribute__ (( packed )) {
  u32_t magic;
  u32_t len;
  u16_t crc;
  u8_t fname[64];
} fw_upgrade_info;


typedef enum {
  BOOTLOADER_UNDEF = 0,
  BOOTLOADER_FLASH_FIRMWARE,
  BOOTLOADER_EXECUTE,
} bootloader_operation;

#define FW_MAGIC                          0xc0defeed

#define BOOTLOADER_SHMEM_UART_UIX         0
#define BOOTLOADER_SHMEM_SPI_FLASH_UIX    1
#define BOOTLOADER_SHMEM_OPERATION_UIX    4
#define BOOTLOADER_SHMEM_SUBOPERATION_UIX 5
#define BOOTLOADER_SHMEM_SPIF_FW_ADDR_UIX 8

void bootloader_execute();
void bootloader_update_fw();

#endif /* BOOTLOADER_EXEC_H_ */

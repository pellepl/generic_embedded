/*
 * linker_symaccess.h
 *
 *  Created on: Mar 22, 2013
 *      Author: petera
 */

#ifndef LINKER_SYMACCESS_H_
#define LINKER_SYMACCESS_H_

/** These are supposed to be provided from linker script */
extern u32_t RAM_START;
extern u32_t RAM_SIZE;
extern u32_t FLASH_SIZE;
#ifdef CONFIG_SHARED_MEM
extern u32_t __shared_memory_address__;
extern u32_t __shared_memory_address_end__;
#endif
extern u32_t __stack_start__;
extern u32_t __stack_end__;
#ifdef CONFIG_BOOTLOADER
extern u32_t _bootloader_text_lma_start;
extern u32_t _bootloader_text_vma_start;
extern u32_t _bootloader_text_vma_end;
extern u32_t _bootloader_data_lma_start;
extern u32_t _bootloader_data_vma_start;
extern u32_t _bootloader_data_vma_end;
#endif

#ifndef RAM_BEGIN
#define RAM_BEGIN             ((void *)&RAM_START)
#endif
#ifndef RAM_END
#define RAM_END               ((void *)((u32_t)&RAM_START + (u32_t)&RAM_SIZE))
#endif

#ifndef FLASH_BEGIN
#define FLASH_BEGIN           ((void *)FLASH_START)
#endif
#ifndef FLASH_END
#define FLASH_END             ((void *)((u32_t)FLASH_START + (u32_t)&FLASH_SIZE))
#endif

#ifdef CONFIG_SHARED_MEM
#define SHARED_MEMORY_ADDRESS ((void *)&__shared_memory_address__)
#define SHARED_MEMORY_END     ((void *)&__shared_memory_address_end__ )
#define STACK_START           ((void *)&__stack_start__)
#define STACK_END             ((void *)&__stack_end__)
#endif

#ifdef CONFIG_BOOTLOADER
#define BOOTLOADER_TEXT_LMA_START  ((void *)&_bootloader_text_lma_start)
#define BOOTLOADER_TEXT_VMA_START  ((void *)&_bootloader_text_vma_start)
#define BOOTLOADER_TEXT_VMA_END    ((void *)&_bootloader_text_vma_end)

#define BOOTLOADER_DATA_LMA_START  ((void *)&_bootloader_data_lma_start)
#define BOOTLOADER_DATA_VMA_START  ((void *)&_bootloader_data_vma_start)
#define BOOTLOADER_DATA_VMA_END    ((void *)&_bootloader_data_vma_end)
#endif

#endif /* LINKER_SYMACCESS_H_ */

/*
 * linker_symaccess.h
 *
 *  Created on: Mar 22, 2013
 *      Author: petera
 */

#ifndef LINKER_SYMACCESS_H_
#define LINKER_SYMACCESS_H_

extern u32_t RAM_START;
extern u32_t RAM_SIZE;

extern u32_t __shared_memory_address__;
extern u32_t __Shared_Memory_Size;
extern u32_t __stack_start__;
extern u32_t __stack_end__;

extern u32_t _bootloader_text_lma_start;
extern u32_t _bootloader_text_vma_start;
extern u32_t _bootloader_text_vma_end;

extern u32_t _bootloader_data_lma_start;
extern u32_t _bootloader_data_vma_start;
extern u32_t _bootloader_data_vma_end;

#define RAM_BEGIN             ((void *)&RAM_START)
#define RAM_END               ((void *)((u32_t)&RAM_START + (u32_t)&RAM_SIZE))

#define SHARED_MEMORY_ADDRESS ((void *)&__shared_memory_address__)
#define SHARED_MEMORY_SIZE    ((void *)&_Shared_Memory_Size)
#define STACK_START           ((void *)&__stack_start__)
#define STACK_END             ((void *)&__stack_end__)

#define BOOTLOADER_TEXT_LMA_START  ((void *)&_bootloader_text_lma_start)
#define BOOTLOADER_TEXT_VMA_START  ((void *)&_bootloader_text_vma_start)
#define BOOTLOADER_TEXT_VMA_END    ((void *)&_bootloader_text_vma_end)

#define BOOTLOADER_DATA_LMA_START  ((void *)&_bootloader_data_lma_start)
#define BOOTLOADER_DATA_VMA_START  ((void *)&_bootloader_data_vma_start)
#define BOOTLOADER_DATA_VMA_END    ((void *)&_bootloader_data_vma_end)

#endif /* LINKER_SYMACCESS_H_ */

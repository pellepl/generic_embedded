  .syntax unified
  .cpu cortex-m3
  .fpu softvfp
  .thumb

  .global __shared_memory_address__
    .word  __shared_memory_address__
  .global _Shared_Memory_Size
    .word _Shared_Memory_Size
  .global __stack_start__
    .word  __stack_start__
  .global __stack_end__
    .word  __stack_end__

  .global _bootloader_text_lma_start
    .word _bootloader_text_lma_start
  .global _bootloader_text_vma_start
    .word _bootloader_text_vma_start
  .global _bootloader_text_vma_end
    .word _bootloader_text_vma_end

  .global _bootloader_data_lma_start
    .word _bootloader_data_lma_start
  .global _bootloader_data_vma_start
    .word _bootloader_data_vma_start
  .global _bootloader_data_vma_end
    .word _bootloader_data_vma_end


  .end

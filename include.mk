ifndef gensysdir
$(warn defaulting path to generic system module, gensysdir variable not set)
gensysdir = ../generic/system
endif

CPATH 	+= ${gensysdir}/src
SPATH	+= ${gensysdir}/src
INC 	+= -I${gensysdir}/src

### architectures and processors

#   stm32
ifeq (1, $(strip $(ARCH_STM32)))
FLAGS	+= -DARCH_STM32

genarchdir = ${gensysdir}/src/arch/stm32

CPATH 	+= ${genarchdir}
SPATH	+= ${genarchdir}
INC 	+= -I${genarchdir}


#     stm32f1
ifeq (1, $(strip $(PROC_STM32F1)))
FLAGS	+= -DPROC_STM32F1

genprocdir = ${genarchdir}/f1

CPATH 	+= ${genprocdir}
SPATH	+= ${genprocdir}
INC 	+= -I${genprocdir}
endif
#     stm32f4
ifeq (1, $(strip $(PROC_STM32F4)))
FLAGS	+= -DPROC_STM32F4

genprocdir = ${genarchdir}/f4

CPATH 	+= ${genprocdir}
SPATH	+= ${genprocdir}
INC 	+= -I${genprocdir}

endif

endif

### general system files

CFILES	+= arch.c
CFILES	+= system.c
CFILES	+= io.c

### CONFIG_MEMOPS - memset, memcpy

ifeq (1, $(strip $(CONFIG_MEMOPS)))
FLAGS	+= -DCONFIG_MEMOPS
ifeq (1, $(strip $(ARCH_STM32)))
SFILES	+= memcpy.s 
SFILES	+= memset.s 
endif
endif

### CONFIG_VARCALL - variadic call by function

ifeq (1, $(strip $(CONFIG_VARCALL)))
FLAGS	+= -DCONFIG_VARCALL
ifeq (1, $(strip $(ARCH_STM32)))
SFILES	+= variadic.s 
endif
endif

### CONFIG_MATH - mathematical functions

ifeq (1, $(strip $(CONFIG_MATH)))
FLAGS	+= -DCONFIG_MATH
ifeq (1, $(strip $(ARCH_STM32)))
SFILES	+= sqrt.s 
endif
CFILES 	+= trig_q.c
endif

### CONFIG_MINIUTILS - print, strlen, other basics

ifeq (1, $(strip $(CONFIG_MINIUTILS)))
FLAGS	+= -DCONFIG_MINIUTILS
CFILES 	+= miniutils.c
endif

### CONFIG_WDOG - watchdog

ifeq (1, $(strip $(CONFIG_WDOG)))
FLAGS	+= -DCONFIG_WDOG
CFILES 	+= wdog.c
endif

### CONFIG_CLI - command line interface

CONFIG_CLI_SYS_OFF ?= 0
CONFIG_CLI_BUS_OFF ?= 0
CONFIG_CLI_DEV_OFF ?= 0
CONFIG_CLI_GPIO_OFF ?= 0
CONFIG_CLI_RTC_OFF ?= 0
CONFIG_CLI_HMC5883L_OFF ?= 0
CONFIG_CLI_ADXL345_OFF ?= 0
CONFIG_CLI_M24M01_OFF ?= 0
CONFIG_CLI_ITG3200_OFF ?= 0

ifeq (1, $(strip $(CONFIG_CLI)))

FLAGS	+= -DCONFIG_CLI
CFILES 	+= cli.c cli_common_menu.c
ifeq (0, $(strip $(CONFIG_CLI_SYS_OFF)))
CFILES 	+= cli_system.c
else
FLAGS += -DCONFIG_CLI_SYS_OFF
endif
ifeq (0, $(strip $(CONFIG_CLI_BUS_OFF)))
CFILES 	+= cli_bus.c
else
FLAGS += -DCONFIG_CLI_BUS_OFF
endif
ifeq (0, $(strip $(CONFIG_CLI_DEV_OFF)))
CFILES 	+= cli_dev.c
else
FLAGS += -DCONFIG_CLI_DEV_OFF
endif
ifeq (1, $(strip $(CONFIG_GPIO)))
ifeq (0, $(strip $(CONFIG_CLI_GPIO_OFF)))
CFILES 	+= cli_gpio.c
else
FLAGS += -DCONFIG_CLI_GPIO_OFF
endif
endif
ifeq (1, $(strip $(CONFIG_RTC)))
ifeq (0, $(strip $(CONFIG_CLI_RTC_OFF)))
CFILES 	+= cli_rtc.c
else
FLAGS += -DCONFIG_CLI_RTC_OFF
endif
endif
ifeq (1, $(strip $(CONFIG_CLI_ADXL345_OFF)))
FLAGS += -DCONFIG_CLI_ADXL345_OFF
endif
ifeq (1, $(strip $(CONFIG_CLI_HMC5883L_OFF)))
FLAGS += -DCONFIG_CLI_HMC5883L_OFF
endif
ifeq (1, $(strip $(CONFIG_CLI_M24M01_OFF)))
FLAGS += -DCONFIG_CLI_M24M01_OFF
endif
ifeq (1, $(strip $(CONFIG_CLI_ITG3200_OFF)))
FLAGS += -DCONFIG_CLI_ITG3200_OFF
endif


endif

### CONFIG_TASK_QUEUE - task queue system

ifeq (1, $(strip $(CONFIG_TASK_QUEUE)))
FLAGS	+= -DCONFIG_TASK_QUEUE
CFILES 	+= taskq.c
endif

### CONFIG_RINGBUFFER - ring buffer

ifeq (1, $(strip $(CONFIG_RINGBUFFER)))
FLAGS	+= -DCONFIG_RINGBUFFER
CFILES 	+= ringbuf.c
endif

### CONFIG_GPIO - gpio driver

ifeq (1, $(strip $(CONFIG_GPIO)))
FLAGS	+= -DCONFIG_GPIO
CFILES 	+= gpio_arch.c
endif


### CONFIG_UART - uart driver

ifeq (1, $(strip $(CONFIG_UART)))
FLAGS	+= -DCONFIG_UART
CFILES	+= uart_driver.c

#   CONFIG_WIFI232 - wifi over serial driver USR_WIFI232B
ifeq (1, $(strip $(CONFIG_WIFI232)))
FLAGS	+= -DCONFIG_WIFI232
CFILES	+= usr_wifi232_driver.c
endif
endif

### CONFIG_I2C - i2c driver

ifeq (1, $(strip $(CONFIG_I2C)))
FLAGS	+= -DCONFIG_I2C
CFILES	+= i2c_driver.c

#   CONFIG_I2C_DEVICE - generic i2c device
ifeq (1, $(strip $(CONFIG_I2C_DEVICE)))
FLAGS	+= -DCONFIG_I2C_DEVICE
CFILES	+= i2c_dev.c
endif

#   CONFIG_LSM303 - acc/magneto sensor driver LSM303
ifeq (1, $(strip $(CONFIG_LSM303)))
FLAGS	+= -DCONFIG_LSM303
CFILES	+= lsm303_driver.c
endif

#   CONFIG_M24M01 - i2c eeprom M24M01
ifeq (1, $(strip $(CONFIG_M24M01)))
FLAGS	+= -DCONFIG_M24M01
CFILES	+= m24m01_driver.c
endif

#   CONFIG_STMPE811 - i2c io expandor
ifeq (1, $(strip $(CONFIG_STMPE811)))
FLAGS	+= -DCONFIG_STMPE811
CFILES	+= stmpe811_driver.c
CFILES	+= stmpe811_handler.c
endif

#   CONFIG_ADXL345 - i2c accelerometer
ifeq (1, $(strip $(CONFIG_ADXL345)))
FLAGS	+= -DCONFIG_ADXL345
CFILES	+= adxl345_driver.c
endif

#   CONFIG_HMC5883L - i2c magnetometer
ifeq (1, $(strip $(CONFIG_HMC5883L)))
FLAGS	+= -DCONFIG_HMC5883L
CFILES	+= hmc5883l_driver.c
endif

#   CONFIG_ITG3200 - i2c magnetometer
ifeq (1, $(strip $(CONFIG_ITG3200)))
FLAGS	+= -DCONFIG_ITG3200
CFILES	+= itg3200_driver.c
endif

endif

### CONFIG_SPI - spi driver

ifeq (1, $(strip $(CONFIG_SPI)))
FLAGS	+= -DCONFIG_SPI
CFILES	+= spi_driver.c

#   CONFIG_SPI_DEVICE - generic spi device
ifeq (1, $(strip $(CONFIG_SPI_DEVICE)))
ifneq (1, $(strip $(CONFIG_TASK_QUEUE)))
$(error "CONFIG_SPI_DEVICE depends on CONFIG_TASK_QUEUE")
endif
FLAGS	+= -DCONFIG_SPI_DEVICE
CFILES	+= spi_dev.c
endif
#   CONFIG_SPI_FLASH - generic spi flash driver
ifeq (1, $(strip $(CONFIG_SPI_FLASH)))
ifneq (1, $(strip $(CONFIG_TASK_QUEUE)))
$(error "CONFIG_SPI_DEVICE depends on CONFIG_TASK_QUEUE")
endif
FLAGS	+= -DCONFIG_SPI_FLASH
CFILES	+= spi_flash.c
endif
#   CONFIG_SPI_FLASH_M25P16 - spi flash driver for M25P16
ifeq (1, $(strip $(CONFIG_SPI_FLASH_M25P16)))
ifneq (1, $(strip $(CONFIG_TASK_QUEUE)))
$(error "CONFIG_SPI_FLASH_M25P16 depends on CONFIG_TASK_QUEUE")
endif
FLAGS	+= -DCONFIG_SPI_FLASH_M25P16
CFILES	+= spi_flash_m25p16.c
endif
#   CONFIG_SPI_DEVICE_OS - generic spi device over os
ifeq (1, $(strip $(CONFIG_SPI_DEVICE_OS)))
ifneq (1, $(strip $(CONFIG_TASK_QUEUE)))
$(error "CONFIG_SPI_DEVICE_OS depends on CONFIG_TASK_QUEUE")
endif
ifneq (1, $(strip $(CONFIG_OS)))
$(error "CONFIG_SPI_DEVICE_OS depends on CONFIG_OS")
endif
FLAGS	+= -DCONFIG_SPI_DEVICE_OS
CFILES	+= spi_dev_os_generic.c
endif
#   CONFIG_SPI_FLASH_OS - generic flash device over os
ifeq (1, $(strip $(CONFIG_SPI_FLASH_OS)))
ifneq (1, $(strip $(CONFIG_TASK_QUEUE)))
$(error "CONFIG_SPI_FLASH_OS depends on CONFIG_TASK_QUEUE")
endif
ifneq (1, $(strip $(CONFIG_OS)))
$(error "CONFIG_SPI_FLASH_OS depends on CONFIG_OS")
endif
FLAGS	+= -DCONFIG_SPI_FLASH_OS
CFILES	+= spi_flash_os.c
endif
endif

### CONFIG_USB_VCD - usb virtual com port driver

ifeq (1, $(strip $(CONFIG_USB_VCD)))
FLAGS	+= -DCONFIG_USB_VCD

ifeq (1, $(strip $(PROC_STM32F1)))
CPATH	+= ${genprocdir}/usb_vcd 
INC		+= -I./${genprocdir}/usb_vcd
CFILES	+= usb_core.c
CFILES	+= usb_init.c
CFILES	+= usb_int.c
CFILES	+= usb_mem.c
CFILES	+= usb_regs.c
CFILES	+= usb_sil.c
CFILES	+= usb_hw_config.c
CFILES	+= usb_desc.c
CFILES	+= usb_endp.c
CFILES	+= usb_istr.c
CFILES	+= usb_prop.c
CFILES	+= usb_pwr.c

endif

ifeq (1, $(strip $(PROC_STM32F4)))
CPATH	+= ${genprocdir}/usb_vcd 
INC		+= -I./${genprocdir}/usb_vcd
CFILES	+= usb_vcp_impl.c
CFILES	+= usb_bsp.c usbd_desc.c usbd_usr.c

stmusbdir = ../stm32f4_lib/STM32_USB-Host-Device_Lib_V2.1.0/Libraries
CPATH	+= ${stmusbdir}/STM32_USB_Device_Library/Class/cdc/src 
CPATH	+= ${stmusbdir}/STM32_USB_Device_Library/Core/src
CPATH	+= ${stmusbdir}/STM32_USB_OTG_Driver/src
INC		+= -I./${stmusbdir}/STM32_USB_Device_Library/Class/cdc/inc 
INC		+= -I./${stmusbdir}/STM32_USB_Device_Library/Core/inc
INC		+= -I./${stmusbdir}/STM32_USB_OTG_Driver/inc 
CFILES	+= usb_core.c 
CFILES	+= usb_dcd.c 
CFILES	+= usb_dcd_int.c 
CFILES	+= usbd_core.c 
CFILES	+= usbd_req.c 
CFILES	+= usbd_ioreq.c 
CFILES	+= usbd_cdc_core_modified.c
endif

endif

### CONFIG_OS - preemptive scheduling

ifeq (1, $(strip $(CONFIG_OS)))
FLAGS	+= -DCONFIG_OS
ifeq (1, $(strip $(ARCH_STM32)))
CFILES	+= os.c 
CFILES	+= list.c 
SFILES	+= svc_handler.s 
endif
endif

### CONFIG_SHARED_MEM - shared memory surviving resets

ifeq (1, $(strip $(CONFIG_SHARED_MEM)))
FLAGS	+= -DCONFIG_SHARED_MEM
ifeq (1, $(strip $(ARCH_STM32)))
SFILES	+= linker_symaccess.s 
endif
CFILES	+= shared_mem.c
endif

### CONFIG_BOOTLOADER - bootloader

ifeq (1, $(strip $(CONFIG_BOOTLOADER)))
ifneq (1, $(strip $(CONFIG_SHARED_MEM)))
$(error "CONFIG_BOOTLOADER depends on CONFIG_SHARED_MEM")
endif
FLAGS	+= -DCONFIG_BOOTLOADER
ifeq (1, $(strip $(ARCH_STM32)))
RFILES	+= bl_exec.c 
RFILES	+= bootloader_hal_flash.c 
ifeq (1, $(strip $(CONFIG_UART)))
RFILES	+= bootloader_hal_uart.c 
endif
ifeq (1, $(strip $(CONFIG_SPI)))
RFILES	+= bootloader_hal_spi.c 
endif
RFILES	+= bootloader.c
endif
endif

### CONFIG_RTC - rtc driver

ifeq (1, $(strip $(CONFIG_RTC)))
FLAGS	+= -DCONFIG_RTC
CFILES	+= rtc_driver.c
endif

### CONFIG_GEN_TIMER - generic timer

ifeq (1, $(strip $(CONFIG_GEN_TIMER)))
FLAGS	+= -DCONFIG_GEN_TIMER
CFILES	+= gen_timer.c 
endif

### CONFIG_NRF905 - nrf905 rf module driver

ifeq (1, $(strip $(CONFIG_NRF905)))
ifneq (1, $(strip $(CONFIG_SPI)))
$(error "CONFIG_NRF905 depends on CONFIG_SPI")
endif
FLAGS	+= -DCONFIG_NRF905
CFILES	+= nrf905_driver.c
endif
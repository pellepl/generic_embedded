/*
 * gpio_stm32f4.c
 *
 *  Created on: Sep 9, 2013
 *      Author: petera
 */

#include "gpio.h"

GPIO_TypeDef *const io_ports[] = {
    GPIOA,
    GPIOB,
    GPIOC,
    GPIOD,
    GPIOE,
    GPIOF,
    GPIOG,
    GPIOH,
    GPIOI
};

u32_t const io_rcc[] = {
    RCC_AHB1Periph_GPIOA,
    RCC_AHB1Periph_GPIOB,
    RCC_AHB1Periph_GPIOC,
    RCC_AHB1Periph_GPIOD,
    RCC_AHB1Periph_GPIOE,
    RCC_AHB1Periph_GPIOF,
    RCC_AHB1Periph_GPIOG,
    RCC_AHB1Periph_GPIOH,
    RCC_AHB1Periph_GPIOI
};

u8_t const io_pinsources[] = {
    GPIO_PinSource0,
    GPIO_PinSource1,
    GPIO_PinSource2,
    GPIO_PinSource3,
    GPIO_PinSource4,
    GPIO_PinSource5,
    GPIO_PinSource6,
    GPIO_PinSource7,
    GPIO_PinSource8,
    GPIO_PinSource9,
    GPIO_PinSource10,
    GPIO_PinSource11,
    GPIO_PinSource12,
    GPIO_PinSource13,
    GPIO_PinSource14,
    GPIO_PinSource15,
};

u16_t const io_pins[] = {
    GPIO_Pin_0,
    GPIO_Pin_1,
    GPIO_Pin_2,
    GPIO_Pin_3,
    GPIO_Pin_4,
    GPIO_Pin_5,
    GPIO_Pin_6,
    GPIO_Pin_7,
    GPIO_Pin_8,
    GPIO_Pin_9,
    GPIO_Pin_10,
    GPIO_Pin_11,
    GPIO_Pin_12,
    GPIO_Pin_13,
    GPIO_Pin_14,
    GPIO_Pin_15,
};

u8_t const io_afs[] = {
    0,
    1,
    2,
    3,
    4,
    5,
    6,
    7,
    8,
    9,
    10,
    11,
    12,
    13,
    14,
    15,
};

GPIOSpeed_TypeDef const io_speeds[] = {
    GPIO_Speed_2MHz,
    GPIO_Speed_25MHz,
    GPIO_Speed_50MHz,
    GPIO_Speed_100MHz
};

GPIOMode_TypeDef const io_modes[] = {
    GPIO_Mode_IN,
    GPIO_Mode_OUT,
    GPIO_Mode_AF,
    GPIO_Mode_AN,
};

GPIOOType_TypeDef const io_outtypes[] = {
    GPIO_OType_PP,
    GPIO_OType_OD,
};

GPIOPuPd_TypeDef const io_pulls[] = {
    GPIO_PuPd_NOPULL,
    GPIO_PuPd_UP,
    GPIO_PuPd_DOWN
};

EXTITrigger_TypeDef const io_flanks[] = {
    EXTI_Trigger_Rising,
    EXTI_Trigger_Falling,
    EXTI_Trigger_Rising_Falling
};

u8_t const io_exti_portsources[] = {
    EXTI_PortSourceGPIOA,
    EXTI_PortSourceGPIOB,
    EXTI_PortSourceGPIOC,
    EXTI_PortSourceGPIOD,
    EXTI_PortSourceGPIOE,
    EXTI_PortSourceGPIOF,
    EXTI_PortSourceGPIOG,
    EXTI_PortSourceGPIOH,
    EXTI_PortSourceGPIOI,
};

u8_t const io_exti_pinsources[] = {
    EXTI_PinSource0,
    EXTI_PinSource1,
    EXTI_PinSource2,
    EXTI_PinSource3,
    EXTI_PinSource4,
    EXTI_PinSource5,
    EXTI_PinSource6,
    EXTI_PinSource7,
    EXTI_PinSource8,
    EXTI_PinSource9,
    EXTI_PinSource10,
    EXTI_PinSource11,
    EXTI_PinSource12,
    EXTI_PinSource13,
    EXTI_PinSource14,
    EXTI_PinSource15,
};

u32_t const io_exti_lines[] = {
    EXTI_Line0,
    EXTI_Line1,
    EXTI_Line2,
    EXTI_Line3,
    EXTI_Line4,
    EXTI_Line5,
    EXTI_Line6,
    EXTI_Line7,
    EXTI_Line8,
    EXTI_Line9,
    EXTI_Line10,
    EXTI_Line11,
    EXTI_Line12,
    EXTI_Line13,
    EXTI_Line14,
    EXTI_Line15,
};

IRQn_Type const io_ext_irq[] = {
    EXTI0_IRQn,
    EXTI1_IRQn,
    EXTI2_IRQn,
    EXTI3_IRQn,
    EXTI4_IRQn,
    EXTI9_5_IRQn,
    EXTI9_5_IRQn,
    EXTI9_5_IRQn,
    EXTI9_5_IRQn,
    EXTI9_5_IRQn,
    EXTI15_10_IRQn,
    EXTI15_10_IRQn,
    EXTI15_10_IRQn,
    EXTI15_10_IRQn,
    EXTI15_10_IRQn,
    EXTI15_10_IRQn,
};

static struct {
  u32_t enabled_pins[_IO_PORTS];
  gpio_interrupt_fn ifns[_IO_PINS];
} _gpio;

static void io_enable_pin(gpio_port port, gpio_pin pin) {
  if (_gpio.enabled_pins[port] == 0) {
    // first pin enabled on port, start port clock
    RCC_AHB1PeriphClockCmd(io_rcc[port], ENABLE);
  }
  _gpio.enabled_pins[port] |= (1<<pin);
}

static void io_disable_pin(gpio_port port, gpio_pin pin) {
  _gpio.enabled_pins[port] &= ~(1<<pin);
  if (_gpio.enabled_pins[port] == 0) {
    // all pins disabled on port, stop port clock
    RCC_AHB1PeriphClockCmd(io_rcc[port], DISABLE);
  }
}

static void io_setup(gpio_port port, gpio_pin pin, io_speed speed, gpio_mode mode, gpio_af af, gpio_outtype outtype, gpio_pull pull) {
  GPIO_InitTypeDef hw;
  if (mode == AF) {
    GPIO_PinAFConfig((GPIO_TypeDef *)io_ports[port], io_pinsources[pin], io_afs[af]);
  }
  hw.GPIO_Pin = io_pins[pin];
  hw.GPIO_Mode = io_modes[mode];
  hw.GPIO_Speed = io_speeds[speed];
  hw.GPIO_OType = io_outtypes[outtype];
  hw.GPIO_PuPd  = io_pulls[pull];
  GPIO_Init((GPIO_TypeDef *)io_ports[port], &hw);
}

void *gpio_get_hw_port(gpio_port port) {
  return io_ports[port];
}

int gpio_get_hw_pin(gpio_pin pin) {
  return io_pins[pin];
}

void gpio_config(gpio_port port, gpio_pin pin, io_speed speed, gpio_mode mode, gpio_af af, gpio_outtype outtype, gpio_pull pull) {
  io_enable_pin(port, pin);
  io_setup(port, pin, speed, mode, af, outtype, pull);
}
void gpio_config_out(gpio_port port, gpio_pin pin, io_speed speed, gpio_outtype outtype, gpio_pull pull) {
  io_enable_pin(port, pin);
  io_setup(port, pin, speed, OUT, AF0, outtype, pull);
}
void gpio_config_in(gpio_port port, gpio_pin pin, io_speed speed) {
  io_enable_pin(port, pin);
  io_setup(port, pin, speed, IN, AF0, PUSHPULL, NOPULL);
}
void gpio_config_analog(gpio_port port, gpio_pin pin) {
  io_enable_pin(port, pin);
  io_setup(port, pin, CLK_100MHZ, IN, AF0, PUSHPULL, NOPULL);
}
void gpio_config_release(gpio_port port, gpio_pin pin) {
  io_disable_pin(port, pin);
}
void gpio_enable(gpio_port port, gpio_pin pin) {
  GPIO_enable(io_ports[port], io_pins[pin]);
}
void gpio_disable(gpio_port port, gpio_pin pin) {
  GPIO_disable(io_ports[port], io_pins[pin]);
}
void gpio_set(gpio_port port, gpio_pin enable_pin, gpio_pin disable_pin) {
  GPIO_set(io_ports[port], io_pins[enable_pin], io_pins[disable_pin]);
}
u32_t gpio_get(gpio_port port, gpio_pin pin) {
  return GPIO_read(io_ports[port], io_pins[pin]);
}
s32_t gpio_interrupt_config(gpio_port port, gpio_pin pin, gpio_interrupt_fn fn, gpio_flank flank) {
  EXTI_InitTypeDef EXTI_InitStructure;

  if (_gpio.ifns[pin]) {
    // already busy
    return -1;
  }
  _gpio.ifns[pin] = fn;


  SYSCFG_EXTILineConfig(io_exti_portsources[port], io_exti_pinsources[pin]);

  EXTI_InitStructure.EXTI_Line = io_exti_lines[pin];
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = io_flanks[flank];
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);

  return 0;
}

void gpio_interrupt_deconfig(gpio_port port, gpio_pin pin) {
  EXTI_InitTypeDef EXTI_InitStructure;
  _gpio.ifns[pin] = NULL;

  NVIC_DisableIRQ(io_ext_irq[pin]);

  EXTI_InitStructure.EXTI_Line = io_exti_lines[pin];
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
  EXTI_InitStructure.EXTI_LineCmd = DISABLE;
  EXTI_Init(&EXTI_InitStructure);
}

void gpio_interrupt_mask_disable(gpio_port port, gpio_pin pin) {
  NVIC_DisableIRQ(io_ext_irq[pin]);
}

void gpio_interrupt_mask_enable(gpio_port port, gpio_pin pin, bool clear_pending) {
  if (clear_pending) {
    EXTI_ClearITPendingBit(io_exti_lines[pin]);
  }
  NVIC_EnableIRQ(io_ext_irq[pin]);
}

void gpio_init(void) {
  memset(&_gpio, 0, sizeof(_gpio));
}

static void _gpio_check_exti(gpio_pin pin) {
  if(EXTI_GetITStatus(io_exti_lines[pin]) != RESET) {
    EXTI_ClearITPendingBit(io_exti_lines[pin]);
    if (_gpio.ifns[pin]) _gpio.ifns[pin](pin);
  }
}

void EXTI0_IRQHandler(void) {
  TRACE_IRQ_ENTER(EXTI0_IRQn);
  _gpio_check_exti(PIN0);
  TRACE_IRQ_EXIT(EXTI0_IRQn);
}
void EXTI1_IRQHandler(void) {
  TRACE_IRQ_ENTER(EXTI1_IRQn);
  _gpio_check_exti(PIN1);
  TRACE_IRQ_EXIT(EXTI1_IRQn);
}
void EXTI2_IRQHandler(void) {
  TRACE_IRQ_ENTER(EXTI2_IRQn);
  _gpio_check_exti(PIN2);
  TRACE_IRQ_EXIT(EXTI2_IRQn);
}
void EXTI3_IRQHandler(void) {
  TRACE_IRQ_ENTER(EXTI3_IRQn);
  _gpio_check_exti(PIN3);
  TRACE_IRQ_EXIT(EXTI3_IRQn);
}
void EXTI4_IRQHandler(void) {
  TRACE_IRQ_ENTER(EXTI4_IRQn);
  _gpio_check_exti(PIN4);
  TRACE_IRQ_EXIT(EXTI4_IRQn);
}
void EXTI9_5_IRQHandler(void) {
  TRACE_IRQ_ENTER(EXTI9_5_IRQn);
  _gpio_check_exti(PIN5);
  _gpio_check_exti(PIN6);
  _gpio_check_exti(PIN7);
  _gpio_check_exti(PIN8);
  _gpio_check_exti(PIN9);
  TRACE_IRQ_EXIT(EXTI9_5_IRQn);
}
void EXTI15_10_IRQHandler(void) {
  TRACE_IRQ_ENTER(EXTI15_10_IRQn);
  _gpio_check_exti(PIN10);
  _gpio_check_exti(PIN11);
  _gpio_check_exti(PIN12);
  _gpio_check_exti(PIN13);
  _gpio_check_exti(PIN14);
  _gpio_check_exti(PIN15);
  TRACE_IRQ_EXIT(EXTI15_10_IRQn);
}

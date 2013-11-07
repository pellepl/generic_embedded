/*
 * system_debug.h
 *
 *  Created on: Apr 23, 2013
 *      Author: petera
 */

#ifndef SYSTEM_DEBUG_H_
#define SYSTEM_DEBUG_H_

/** ASSERTS **/

#ifdef ASSERT_OFF
#define ASSERT(x) do {} while(0);
#else
#define ASSERT(x) \
do { \
  if (!(x)) {\
    SYS_assert(__FILE__, __LINE__);\
  }\
} while (0);
#endif

/** DEBUG PRINTS **/

#ifndef DBG_OFF

#define DBG_TIMESTAMP_PREFIX   1
#define DBG_LEVEL_PREFIX       0

#define _DBG_BIT_NAMES { \
  "sys",\
  "app",\
  "task",\
  "os",\
  "heap",\
  "comm",\
  "cli",\
  "nvs", \
  "spi", \
  "eth", \
  "fs", \
  "i2c", \
  "wifi", \
  "web", \
}

#ifdef DBG_SYS_OFF
#define D_SYS     0
#else
#define D_SYS     (1<<0)
#endif
#ifdef DBG_APP_OFF
#define D_APP     0
#else
#define D_APP     (1<<1)
#endif
#ifdef DBG_TASK_OFF
#define D_TASK    0
#else
#define D_TASK    (1<<2)
#endif
#ifdef DBG_OS_OFF
#define D_OS      0
#else
#define D_OS      (1<<3)
#endif
#ifdef DBG_HEAP_OFF
#define D_HEAP    0
#else
#define D_HEAP    (1<<4)
#endif
#ifdef DBG_COMM_OFF
#define D_COMM    0
#else
#define D_COMM    (1<<5)
#endif
#ifdef DBG_CLI_OFF
#define D_CLI 0
#else
#define D_CLI     (1<<6)
#endif
#ifdef DBG_NVS_OFF
#define D_NVS     0
#else
#define D_NVS     (1<<7)
#endif
#ifdef DBG_SPI_OFF
#define D_SPI     0
#else
#define D_SPI     (1<<8)
#endif
#ifdef DBG_ETH_OFF
#define D_ETH     0
#else
#define D_ETH     (1<<9)
#endif
#ifdef DBG_FS_OFF
#define D_FS     0
#else
#define D_FS      (1<<10)
#endif
#ifdef DBG_I2C_OFF
#define D_I2C    0
#else
#define D_I2C     (1<<11)
#endif
#ifdef DBG_WIFI_OFF
#define D_WIFI   0
#else
#define D_WIFI    (1<<12)
#endif
#ifdef DBG_WEB_OFF
#define D_WEB   0
#else
#define D_WEB     (1<<13)
#endif
#ifdef DBG_ANY_OFF
#define D_ANY     0
#else
#define D_ANY     (0xffffffff)
#endif

#else // DBG_OFF

#define D_SYS     0
#define D_APP     0
#define D_TASK    0
#define D_OS      0
#define D_HEAP    0
#define D_COMM    0
#define D_CLI     0
#define D_NVS     0
#define D_SPI     0
#define D_ETH     0
#define D_FS      0
#define D_I2C     0
#define D_WIFI    0
#define D_ANY     0
#define D_WEB     0

#endif  // DBG_OFF

#define D_DEBUG   0
#define D_INFO    1
#define D_WARN    2
#define D_FATAL   3

// want to keep DBG as macro giving compiler opportunity to optimize away all
// occurrences of DBG masks being 0
#if DBG_LEVEL_PREFIX
extern const char* __dbg_level_str[4];
#define DBG_LEVEL_PRINT(level) \
  switch (level) { case 0:case 1:print("%s ", __dbg_level_str[level]);break; \
                    case 2:print(TEXT_NOTE("%s "),__dbg_level_str[level]);break;\
                    case 3:print(TEXT_BAD("%s "),__dbg_level_str[level]);break;}
#else
#define DBG_LEVEL_PRINT(level)
#endif

#ifdef DBG_OFF
#define DBG(mask, level, f, ...) do {} while(0);
#define IF_DBG(mask, level) while (0)
#else
#define DBG(mask, level, f, ...) do { \
     if (((mask) & __dbg_mask) && level >= __dbg_level) { \
       if (DBG_TIMESTAMP_PREFIX) { \
         u8_t __hh; u8_t __mm; u8_t __ss; u16_t __mil; \
         SYS_get_time(NULL, &__hh, &__mm, &__ss, &__mil); \
         print("%02i:%02i:%02i.%03i ", __hh, __mm, __ss, __mil); \
       } \
       if (DBG_LEVEL_PREFIX) { DBG_LEVEL_PRINT(level); } \
       print((f), ## __VA_ARGS__); \
     } \
  } while (0)
#define IF_DBG(mask, level) if (((mask) & __dbg_mask) && level >= __dbg_level)
#endif



/** MONITORING **/

#ifdef DBG_TRACE_MON
#ifndef TRACE_SIZE
#error "TRACE_SIZE must be defined when DBG_TRACE_MON is enabled"
#endif
extern u16_t _trace_log[];
extern volatile u32_t _trace_log_ix;
extern volatile bool __trace;

#define TRACE_LOG(op, var)   \
  if (__trace) do { _trace_log[_trace_log_ix] = (((op)<<8)|((var)&0xff)); \
    _trace_log_ix = (_trace_log_ix >= TRACE_SIZE-1) ? 0 : (_trace_log_ix + 1); \
  } while (0);


#define TRACE_LOG_MS(op, var)   \
  if (__trace) do {  \
    u16_t tmp = _trace_log[(_trace_log_ix-1)&(TRACE_SIZE-1)]; \
    if ((tmp >> 8) != _TRC_OP_MS_TICK) { \
      _trace_log[_trace_log_ix] = ((op)<<8); \
      _trace_log_ix = (_trace_log_ix >= TRACE_SIZE-1) ? 0 : (_trace_log_ix + 1); \
    } else if ((tmp & 0xff) < 0xff) { \
      _trace_log[(_trace_log_ix-1)&(TRACE_SIZE-1)] = (_TRC_OP_MS_TICK<<8) | (((tmp & 0xff) + 1) & 0xff); \
    } \
  } while (0);

#define TRACE_SET(x)    __trace_set(x);
#define TRACE_START(x)  __trace_start(x);
#define TRACE_STOP(x)   __trace_stop(x);

bool __trace_start(void);
bool __trace_stop(void);
bool __trace_set(bool x);

// trace irq definitions

#ifdef ARCH_STM32

#ifdef PROC_STM32F1

static const char* const TRACE_IRQ_NAMES[] = {
"SVCall_IRQn",
"DebugMonitor_IRQn",
"???",
"PendSV_IRQn",
"SysTick_IRQn",
"WWDG_IRQn",
"PVD_IRQn",
"TAMPER_IRQn",
"RTC_IRQn",
"FLASH_IRQn",
"RCC_IRQn",
"EXTI0_IRQn",
"EXTI1_IRQn",
"EXTI2_IRQn",
"EXTI3_IRQn",
"EXTI4_IRQn",
"DMA1_Channel1_IRQn",
"DMA1_Channel2_IRQn",
"DMA1_Channel3_IRQn",
"DMA1_Channel4_IRQn",
"DMA1_Channel5_IRQn",
"DMA1_Channel6_IRQn",
"DMA1_Channel7_IRQn",
#ifdef STM32F10X_LD
"ADC1_2_IRQn",
"USB_HP_CAN1_TX_IRQn",
"USB_LP_CAN1_RX0_IRQn",
"CAN1_RX1_IRQn",
"CAN1_SCE_IRQn",
"EXTI9_5_IRQn",
"TIM1_BRK_IRQn",
"TIM1_UP_IRQn",
"TIM1_TRG_COM_IRQn",
"TIM1_CC_IRQn",
"TIM2_IRQn",
"TIM3_IRQn",
"I2C1_EV_IRQn",
"I2C1_ER_IRQn",
"SPI1_IRQn",
"USART1_IRQn",
"USART2_IRQn",
"EXTI15_10_IRQn",
"RTCAlarm_IRQn",
"USBWakeUp_IRQn",
#endif /* STM32F10X_LD */
#ifdef STM32F10X_LD_VL
"ADC1_IRQn",
"EXTI9_5_IRQn",
"TIM1_BRK_TIM15_IRQn",
"TIM1_UP_TIM16_IRQn",
"TIM1_TRG_COM_TIM17_IRQn",
"TIM1_CC_IRQn",
"TIM2_IRQn",
"TIM3_IRQn",
"I2C1_EV_IRQn",
"I2C1_ER_IRQn",
"SPI1_IRQn",
"USART1_IRQn",
"USART2_IRQn",
"EXTI15_10_IRQn",
"RTCAlarm_IRQn",
"CEC_IRQn",
"TIM6_DAC_IRQn",
"TIM7_IRQn",
#endif /* STM32F10X_LD_VL */
#ifdef STM32F10X_MD
"ADC1_2_IRQn",
"USB_HP_CAN1_TX_IRQn",
"USB_LP_CAN1_RX0_IRQn",
"CAN1_RX1_IRQn",
"CAN1_SCE_IRQn",
"EXTI9_5_IRQn",
"TIM1_BRK_IRQn",
"TIM1_UP_IRQn",
"TIM1_TRG_COM_IRQn",
"TIM1_CC_IRQn",
"TIM2_IRQn",
"TIM3_IRQn",
"TIM4_IRQn",
"I2C1_EV_IRQn",
"I2C1_ER_IRQn",
"I2C2_EV_IRQn",
"I2C2_ER_IRQn",
"SPI1_IRQn",
"SPI2_IRQn",
"USART1_IRQn",
"USART2_IRQn",
"USART3_IRQn",
"EXTI15_10_IRQn",
"RTCAlarm_IRQn",
"USBWakeUp_IRQn",
#endif /* STM32F10X_MD */
#ifdef STM32F10X_MD_VL
"ADC1_IRQn",
"EXTI9_5_IRQn",
"TIM1_BRK_TIM15_IRQn",
"TIM1_UP_TIM16_IRQn",
"TIM1_TRG_COM_TIM17_IRQn",
"TIM1_CC_IRQn",
"TIM2_IRQn",
"TIM3_IRQn",
"TIM4_IRQn",
"I2C1_EV_IRQn",
"I2C1_ER_IRQn",
"I2C2_EV_IRQn",
"I2C2_ER_IRQn",
"SPI1_IRQn",
"SPI2_IRQn",
"USART1_IRQn",
"USART2_IRQn",
"USART3_IRQn",
"EXTI15_10_IRQn",
"RTCAlarm_IRQn",
"CEC_IRQn",
"TIM6_DAC_IRQn",
"TIM7_IRQn",
#endif /* STM32F10X_MD_VL */
#ifdef STM32F10X_HD
"ADC1_2_IRQn",
"USB_HP_CAN1_TX_IRQn",
"USB_LP_CAN1_RX0_IRQn",
"CAN1_RX1_IRQn",
"CAN1_SCE_IRQn",
"EXTI9_5_IRQn",
"TIM1_BRK_IRQn",
"TIM1_UP_IRQn",
"TIM1_TRG_COM_IRQn",
"TIM1_CC_IRQn",
"TIM2_IRQn",
"TIM3_IRQn",
"TIM4_IRQn",
"I2C1_EV_IRQn",
"I2C1_ER_IRQn",
"I2C2_EV_IRQn",
"I2C2_ER_IRQn",
"SPI1_IRQn",
"SPI2_IRQn",
"USART1_IRQn",
"USART2_IRQn",
"USART3_IRQn",
"EXTI15_10_IRQn",
"RTCAlarm_IRQn",
"USBWakeUp_IRQn",
"TIM8_BRK_IRQn",
"TIM8_UP_IRQn",
"TIM8_TRG_COM_IRQn",
"TIM8_CC_IRQn",
"ADC3_IRQn",
"FSMC_IRQn",
"SDIO_IRQn",
"TIM5_IRQn",
"SPI3_IRQn",
"UART4_IRQn",
"UART5_IRQn",
"TIM6_IRQn",
"TIM7_IRQn",
"DMA2_Channel1_IRQn",
"DMA2_Channel2_IRQn",
"DMA2_Channel3_IRQn",
"DMA2_Channel4_5_IRQn",
#endif /* STM32F10X_HD */
#ifdef STM32F10X_HD_VL
"ADC1_IRQn",
"EXTI9_5_IRQn",
"TIM1_BRK_TIM15_IRQn",
"TIM1_UP_TIM16_IRQn",
"TIM1_TRG_COM_TIM17_IRQn",
"TIM1_CC_IRQn",
"TIM2_IRQn",
"TIM3_IRQn",
"TIM4_IRQn",
"I2C1_EV_IRQn",
"I2C1_ER_IRQn",
"I2C2_EV_IRQn",
"I2C2_ER_IRQn",
"SPI1_IRQn",
"SPI2_IRQn",
"USART1_IRQn",
"USART2_IRQn",
"USART3_IRQn",
"EXTI15_10_IRQn",
"RTCAlarm_IRQn",
"CEC_IRQn",
"TIM12_IRQn",
"TIM13_IRQn",
"TIM14_IRQn",
"TIM5_IRQn",
"SPI3_IRQn",
"UART4_IRQn",
"UART5_IRQn",
"TIM6_DAC_IRQn",
"TIM7_IRQn",
"DMA2_Channel1_IRQn",
"DMA2_Channel2_IRQn",
"DMA2_Channel3_IRQn",
"DMA2_Channel4_5_IRQn",
"DMA2_Channel5_IRQn",
#endif /* STM32F10X_HD_VL */
#ifdef STM32F10X_XL
"ADC1_2_IRQn",
"USB_HP_CAN1_TX_IRQn",
"USB_LP_CAN1_RX0_IRQn",
"CAN1_RX1_IRQn",
"CAN1_SCE_IRQn",
"EXTI9_5_IRQn",
"TIM1_BRK_TIM9_IRQn",
"TIM1_UP_TIM10_IRQn",
"TIM1_TRG_COM_TIM11_IRQn",
"TIM1_CC_IRQn",
"TIM2_IRQn",
"TIM3_IRQn",
"TIM4_IRQn",
"I2C1_EV_IRQn",
"I2C1_ER_IRQn",
"I2C2_EV_IRQn",
"I2C2_ER_IRQn",
"SPI1_IRQn",
"SPI2_IRQn",
"USART1_IRQn",
"USART2_IRQn",
"USART3_IRQn",
"EXTI15_10_IRQn",
"RTCAlarm_IRQn",
"USBWakeUp_IRQn",
"TIM8_BRK_TIM12_IRQn",
"TIM8_UP_TIM13_IRQn",
"TIM8_TRG_COM_TIM14_IRQn",
"TIM8_CC_IRQn",
"ADC3_IRQn",
"FSMC_IRQn",
"SDIO_IRQn",
"TIM5_IRQn",
"SPI3_IRQn",
"UART4_IRQn",
"UART5_IRQn",
"TIM6_IRQn",
"TIM7_IRQn",
"DMA2_Channel1_IRQn",
"DMA2_Channel2_IRQn",
"DMA2_Channel3_IRQn",
"DMA2_Channel4_5_IRQn",
#endif /* STM32F10X_XL */
#ifdef STM32F10X_CL
"ADC1_2_IRQn",
"CAN1_TX_IRQn",
"CAN1_RX0_IRQn",
"CAN1_RX1_IRQn",
"CAN1_SCE_IRQn",
"EXTI9_5_IRQn",
"TIM1_BRK_IRQn",
"TIM1_UP_IRQn",
"TIM1_TRG_COM_IRQn",
"TIM1_CC_IRQn",
"TIM2_IRQn",
"TIM3_IRQn",
"TIM4_IRQn",
"I2C1_EV_IRQn",
"I2C1_ER_IRQn",
"I2C2_EV_IRQn",
"I2C2_ER_IRQn",
"SPI1_IRQn",
"SPI2_IRQn",
"USART1_IRQn",
"USART2_IRQn",
"USART3_IRQn",
"EXTI15_10_IRQn",
"RTCAlarm_IRQn",
"OTG_FS_WKUP_IRQn",
"TIM5_IRQn",
"SPI3_IRQn",
"UART4_IRQn",
"UART5_IRQn",
"TIM6_IRQn",
"TIM7_IRQn",
"DMA2_Channel1_IRQn",
"DMA2_Channel2_IRQn",
"DMA2_Channel3_IRQn",
"DMA2_Channel4_IRQn",
"DMA2_Channel5_IRQn",
"ETH_IRQn",
"ETH_WKUP_IRQn",
"CAN2_TX_IRQn",
"CAN2_RX0_IRQn",
"CAN2_RX1_IRQn",
"CAN2_SCE_IRQn",
"OTG_FS_IRQn",
#endif /* STM32F10X_CL */
};
#endif // PROC_STM32F1

#ifdef PROC_STM32F4

static const char* const TRACE_IRQ_NAMES[] = {
  "SVCall_IRQn",                 /*-5*/
  "DebugMonitor_IRQn",           /*-4*/
  "???",                         /*-3*/
  "PendSV_IRQn",                 /*-2*/
  "SysTick_IRQn",                /*-1*/
  "WWDG_IRQn",                   /*0*/
  "PVD_IRQn",                    /*1*/
  "TAMP_STAMP_IRQn",             /*2*/
  "RTC_WKUP_IRQn",               /*3*/
  "FLASH_IRQn",                  /*4*/
  "RCC_IRQn",                    /*5*/
  "EXTI0_IRQn",                  /*6*/
  "EXTI1_IRQn",                  /*7*/
  "EXTI2_IRQn",                  /*8*/
  "EXTI3_IRQn",                  /*9*/
  "EXTI4_IRQn",                  /*10*/
  "DMA1_Stream0_IRQn",           /*11*/
  "DMA1_Stream1_IRQn",           /*12*/
  "DMA1_Stream2_IRQn",           /*13*/
  "DMA1_Stream3_IRQn",           /*14*/
  "DMA1_Stream4_IRQn",           /*15*/
  "DMA1_Stream5_IRQn",           /*16*/
  "DMA1_Stream6_IRQn",           /*17*/
  "ADC_IRQn",                    /*18*/
  "CAN1_TX_IRQn",                /*19*/
  "CAN1_RX0_IRQn",               /*20*/
  "CAN1_RX1_IRQn",               /*21*/
  "CAN1_SCE_IRQn",               /*22*/
  "EXTI9_5_IRQn",                /*23*/
  "TIM1_BRK_TIM9_IRQn",          /*24*/
  "TIM1_UP_TIM10_IRQn",          /*25*/
  "TIM1_TRG_COM_TIM11_IRQn",     /*26*/
  "TIM1_CC_IRQn",                /*27*/
  "TIM2_IRQn",                   /*28*/
  "TIM3_IRQn",                   /*29*/
  "TIM4_IRQn",                   /*30*/
  "I2C1_EV_IRQn",                /*31*/
  "I2C1_ER_IRQn",                /*32*/
  "I2C2_EV_IRQn",                /*33*/
  "I2C2_ER_IRQn",                /*34*/
  "SPI1_IRQn",                   /*35*/
  "SPI2_IRQn",                   /*36*/
  "USART1_IRQn",                 /*37*/
  "USART2_IRQn",                 /*38*/
  "USART3_IRQn",                 /*39*/
  "EXTI15_10_IRQn",              /*40*/
  "RTC_Alarm_IRQn",              /*41*/
  "OTG_FS_WKUP_IRQn",            /*42*/
  "TIM8_BRK_TIM12_IRQn",         /*43*/
  "TIM8_UP_TIM13_IRQn",          /*44*/
  "TIM8_TRG_COM_TIM14_IRQn",     /*45*/
  "TIM8_CC_IRQn",                /*46*/
  "DMA1_Stream7_IRQn",           /*47*/
  "FSMC_IRQn",                   /*48*/
  "SDIO_IRQn",                   /*49*/
  "TIM5_IRQn",                   /*50*/
  "SPI3_IRQn",                   /*51*/
  "UART4_IRQn",                  /*52*/
  "UART5_IRQn",                  /*53*/
  "TIM6_DAC_IRQn",               /*54*/
  "TIM7_IRQn",                   /*55*/
  "DMA2_Stream0_IRQn",           /*56*/
  "DMA2_Stream1_IRQn",           /*57*/
  "DMA2_Stream2_IRQn",           /*58*/
  "DMA2_Stream3_IRQn",           /*59*/
  "DMA2_Stream4_IRQn",           /*60*/
  "ETH_IRQn",                    /*61*/
  "ETH_WKUP_IRQn",               /*62*/
  "CAN2_TX_IRQn",                /*63*/
  "CAN2_RX0_IRQn",               /*64*/
  "CAN2_RX1_IRQn",               /*65*/
  "CAN2_SCE_IRQn",               /*66*/
  "OTG_FS_IRQn",                 /*67*/
  "DMA2_Stream5_IRQn",           /*68*/
  "DMA2_Stream6_IRQn",           /*69*/
  "DMA2_Stream7_IRQn",           /*70*/
  "USART6_IRQn",                 /*71*/
  "I2C3_EV_IRQn",                /*72*/
  "I2C3_ER_IRQn",                /*73*/
  "OTG_HS_EP1_OUT_IRQn",         /*74*/
  "OTG_HS_EP1_IN_IRQn",          /*75*/
  "OTG_HS_WKUP_IRQn",            /*76*/
  "OTG_HS_IRQn",                 /*77*/
  "DCMI_IRQn",                   /*78*/
  "CRYP_IRQn",                   /*79*/
  "HASH_RNG_IRQn",               /*80*/
#ifdef STM32F40XX
  "FPU_IRQn",                    /*81*/
#endif /* STM32F40XX */
#ifdef STM32F427X
  "FPU_IRQn",                    /*81*/
  "UART7_IRQn",                  /*82*/
  "UART8_IRQn",                  /*83*/
  "SPI4_IRQn",                   /*84*/
  "SPI5_IRQn",                   /*85*/
  "SPI6_IRQn",                   /*86*/
#endif /* STM32F427X */
};
#endif // PROC_STM32F4
#else // ARCH_STM32
#error "undefined arch for trace"
#endif // ARCH_STM32
#else // DBG_TRACE_MON
#define TRACE_LOG(op, var)
#define TRACE_LOG_MS(op, var)
#define TRACE_SET(x)    (void)(x)
#define TRACE_START(x)  FALSE
#define TRACE_STOP(x)   FALSE
#endif // DBG_TRACE_MON

typedef enum {
  _BAD = 0,
  _TRC_OP_MS_TICK = 1,
  _TRC_OP_IRQ_ENTER, _TRC_OP_IRQ_EXIT,
  _TRC_OP_IRQ_ON, _TRC_OP_IRQ_OFF,
  _TRC_OP_OS_CTX_LEAVE, _TRC_OP_OS_CTX_ENTER,
  _TRC_OP_OS_CREATE, _TRC_OP_OS_YIELD, _TRC_OP_OS_THRSLEEP, _TRC_OP_OS_DEAD,
  _TRC_OP_OS_MUTLOCK, _TRC_OP_OS_MUTACQ_LOCK, _TRC_OP_OS_MUTWAIT_LOCK, _TRC_OP_OS_MUTUNLOCK,
  _TRC_OP_OS_CONDWAIT, _TRC_OP_OS_CONDTIMWAIT, _TRC_OP_OS_CONDSIG, _TRC_OP_OS_SIGWAKED, _TRC_OP_OS_CONDBROAD, _TRC_OP_OS_CONDTIMWAKED,
  _TRC_OP_OS_THRWAKED, _TRC_OP_OS_SLEEP,
  _TRC_OP_OS_PREEMPTION,
  _TRC_OP_TASK_RUN, _TRC_OP_TASK_ENTER, _TRC_OP_TASK_EXIT, _TRC_OP_TASK_TIMER,
  _TRC_OP_USER_MSG,
} _trc_types ;

#ifdef DBG_TRACE_MON

static const char* const TRACE_NAMES[] = {
  "<>",
  "ms_tick",
  "irq_enter","irq_exit ",
  "irq_on", "irq_off",
  "ctx_leave", "ctx_enter",
  "thr_create", "thr_yield", "thr_sleep", "thr_dead",
  "mutex_lock", "mutex_acquire", "mutex_wait", "mutex_unlock",
  "cond_wait", "cond_tim_wait", "cond_signal", "cond_sig_waked", "cond_broadcast", "cond_tim_waked",
  "thr_wakeup", "os_sleep",
  "preemption",
  "task_run", "task_enter", "task_exit ", "task_timer",
  "user_msg"
};

#endif // DBG_TRACE_MON

#define TRACE_MS_TICK(ms)           TRACE_LOG_MS(_TRC_OP_MS_TICK, ms)

#ifdef ARCH_STM32
#define TRACE_IRQ_ENTER(irq)        TRACE_LOG(_TRC_OP_IRQ_ENTER, irq+5)
#define TRACE_IRQ_EXIT(irq)         TRACE_LOG(_TRC_OP_IRQ_EXIT, irq+5)
#else
#define TRACE_IRQ_ENTER(irq)        TRACE_LOG(_TRC_OP_IRQ_ENTER, (irq))
#define TRACE_IRQ_ENTER(irq)        TRACE_LOG(_TRC_OP_IRQ_EXIT, (irq))
#endif

#define TRACE_IRQ_ON(l)             //TRACE_LOG(_TRC_OP_IRQ_ON, l)
#define TRACE_IRQ_OFF(l)            //TRACE_LOG(_TRC_OP_IRQ_OFF, l)

#define TRACE_OS_CTX_LEAVE(thr)     TRACE_LOG(_TRC_OP_OS_CTX_LEAVE, (thr)->id)
#define TRACE_OS_CTX_ENTER(thr)     TRACE_LOG(_TRC_OP_OS_CTX_ENTER, (thr)->id)
#define TRACE_OS_THRCREATE(thr)     TRACE_LOG(_TRC_OP_OS_CREATE, (thr)->id)
#define TRACE_OS_YIELD(thr)         TRACE_LOG(_TRC_OP_OS_YIELD, (thr)->id)
#define TRACE_OS_THRDEAD(thr)       TRACE_LOG(_TRC_OP_OS_DEAD, (thr)->id)
#define TRACE_OS_THRSLEEP(thr)      TRACE_LOG(_TRC_OP_OS_THRSLEEP, (thr)->id)
#define TRACE_OS_MUTLOCK(m)         TRACE_LOG(_TRC_OP_OS_MUTLOCK, (m)->id)
#define TRACE_OS_MUT_ACQLOCK(thr)   TRACE_LOG(_TRC_OP_OS_MUTACQ_LOCK, (thr)->id)
#define TRACE_OS_MUT_WAITLOCK(thr)  TRACE_LOG(_TRC_OP_OS_MUTWAIT_LOCK, (thr)->id)
#define TRACE_OS_MUTUNLOCK(m)       TRACE_LOG(_TRC_OP_OS_MUTUNLOCK, (m)->id)
#define TRACE_OS_CONDWAIT(c)        TRACE_LOG(_TRC_OP_OS_CONDWAIT, (c)->id)
#define TRACE_OS_CONDTIMWAIT(c)     TRACE_LOG(_TRC_OP_OS_CONDTIMWAIT, (c)->id)
#define TRACE_OS_CONDSIG(c)         TRACE_LOG(_TRC_OP_OS_CONDSIG, (c)->id)
#define TRACE_OS_SIGWAKED(thr)      TRACE_LOG(_TRC_OP_OS_SIGWAKED, (thr)->id)
#define TRACE_OS_CONDBROAD(c)       TRACE_LOG(_TRC_OP_OS_CONDBROAD, (c)->id)
#define TRACE_OS_CONDTIMWAKED(c)    TRACE_LOG(_TRC_OP_OS_CONDTIMWAKED, (c)->id)
#define TRACE_OS_THRWAKED(thr)      TRACE_LOG(_TRC_OP_OS_THRWAKED, (thr)->id)
#define TRACE_OS_SLEEP(x)           TRACE_LOG(_TRC_OP_OS_SLEEP, x)
#define TRACE_OS_PREEMPTION(x)      TRACE_LOG(_TRC_OP_OS_PREEMPTION, x)

#define TRACE_TASK_RUN(x)           TRACE_LOG(_TRC_OP_TASK_RUN, x)
#define TRACE_TASK_ENTER(x)         TRACE_LOG(_TRC_OP_TASK_ENTER, x)
#define TRACE_TASK_EXIT(x)          TRACE_LOG(_TRC_OP_TASK_EXIT, x)
#define TRACE_TASK_TIMER(x)         TRACE_LOG(_TRC_OP_TASK_TIMER, x)


#define TRACE_USR_MSG(m)            TRACE_LOG(_TRC_OP_USER_MSG, m)

/** OS **/

// Enable os debug functionality
#if OS_DBG_MON
// Max thread peer elements
#define OS_THREAD_PEERS       8
// Max mutex peer elements
#define OS_MUTEX_PEERS        32
// Max cond peer elements
#define OS_COND_PEERS         16

// Enable os dump by external interrupt
#define OS_DUMP_IRQ

#ifdef OS_DUMP_IRQ
#define OS_DUMP_IRQ_GPIO_PORT         GPIOE
#define OS_DUMP_IRQ_GPIO_PIN          GPIO_Pin_2
#define OS_DUMP_IRQ_GPIO_PORT_SOURCE  GPIO_PortSourceGPIOE
#define OS_DUMP_IRQ_GPIO_PIN_SOURCE   GPIO_PinSource2
#define OS_DUMP_IRQ_EXTI_LINE         EXTI_Line2
#define OS_DUMP_IRQ_EXTI_IRQn         EXTI2_IRQn
#endif

#endif

// App wise

extern volatile u32_t __dbg_mask;
extern volatile u32_t __dbg_level;

void SYS_dbg_mask_set(u32_t mask);
void SYS_dbg_mask_enable(u32_t mask);
void SYS_dbg_mask_disable(u32_t mask);
void SYS_dbg_level(u32_t level);
u32_t SYS_dbg_get_mask();
u32_t SYS_dbg_get_level();


#endif /* SYSTEM_DEBUG_H_ */

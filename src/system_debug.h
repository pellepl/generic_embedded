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
extern u16_t _trace_log[TRACE_SIZE];
extern volatile u32_t _trace_log_ix;

#define TRACE_LOG(op, var)   \
  do { _trace_log[_trace_log_ix] = (((op)<<8)|((var)&0xff)); \
    _trace_log_ix = (_trace_log_ix >= TRACE_SIZE-1) ? 0 : (_trace_log_ix + 1); \
  } while (0);


#define TRACE_LOG_MS(op, var)   \
  do {  \
    u16_t tmp = _trace_log[(_trace_log_ix-1)&(TRACE_SIZE-1)]; \
    if ((tmp >> 8) != _TRC_OP_MS_TICK) { \
      _trace_log[_trace_log_ix] = ((op)<<8); \
      _trace_log_ix = (_trace_log_ix >= TRACE_SIZE-1) ? 0 : (_trace_log_ix + 1); \
    } else if ((tmp & 0xff) < 0xff) { \
      _trace_log[(_trace_log_ix-1)&(TRACE_SIZE-1)] = (_TRC_OP_MS_TICK<<8) | (((tmp & 0xff) + 1) & 0xff); \
    } \
  } while (0);


#define TRACE_NAMES \
    { "<>", \
  "ms_tick", \
  "irq_enter","irq_exit ", \
  "irq_on", "irq_off", \
  "ctx_leave", "ctx_enter", \
  "thr_create", "thr_yield", "thr_sleep", "thr_dead", \
  "mutex_lock", "mutex_acquire", "mutex_wait", "mutex_unlock", \
  "cond_wait", "cond_tim_wait", "cond_signal", "cond_sig_waked", "cond_broadcast", "cond_tim_waked", \
  "thr_wakeup", "os_sleep", \
  "preemption", \
  "task_run", "task_enter", "task_exit ", "task_timer", \
  "user_msg" \
    }

#ifdef ARCH_STM32
#define TRACE_IRQ_NAMES \
{ "<>", \
  "UART1",        "UART2",        "UART3",        "UART4", \
  "TIM2",         "DMA_SPI1_RX",  "DMA_SPI1_TX",  "DMA_SPI2_RX", \
  "DMA_SPI2_RX",  "EXTI_ETH_SPI", "EXTI_DBG",     "PENDSV", \
  "SYSTICK",      "I2C_ER",       "I2C_EV",       "USB_WKU", \
  "USB_LP", \
}
#else
#error "undefined arch for trace"
#endif
#else
#define TRACE_LOG(op, var)
#define TRACE_LOG_MS(op, var)
#endif

typedef enum {
  _BAD = 0,
  _TRC_OP_MS_TICK = 1,
  _TRC_OP_IRQ_ENTER,
  _TRC_OP_IRQ_EXIT,
  _TRC_OP_IRQ_ON,
  _TRC_OP_IRQ_OFF,
  _TRC_OP_OS_CTX_LEAVE,
  _TRC_OP_OS_CTX_ENTER,
  _TRC_OP_OS_CREATE,
  _TRC_OP_OS_YIELD,
  _TRC_OP_OS_THRSLEEP,
  _TRC_OP_OS_DEAD,
  _TRC_OP_OS_MUTLOCK,
  _TRC_OP_OS_MUTACQ_LOCK,
  _TRC_OP_OS_MUTWAIT_LOCK,
  _TRC_OP_OS_MUTUNLOCK,
  _TRC_OP_OS_CONDWAIT,
  _TRC_OP_OS_CONDTIMWAIT,
  _TRC_OP_OS_CONDSIG,
  _TRC_OP_OS_SIGWAKED,
  _TRC_OP_OS_CONDBROAD,
  _TRC_OP_OS_CONDTIMWAKED,
  _TRC_OP_OS_THRWAKED,
  _TRC_OP_OS_SLEEP,
  _TRC_OP_OS_PREEMPTION,
  _TRC_OP_TASK_RUN,
  _TRC_OP_TASK_ENTER,
  _TRC_OP_TASK_EXIT,
  _TRC_OP_TASK_TIMER,
  _TRC_OP_USER_MSG,
} _trc_types ;

#define TRACE_MS_TICK(ms)           TRACE_LOG_MS(_TRC_OP_MS_TICK, ms)

#define TRACE_IRQ_ENTER(irq)        TRACE_LOG(_TRC_OP_IRQ_ENTER, irq)
#define TRACE_IRQ_EXIT(irq)         TRACE_LOG(_TRC_OP_IRQ_EXIT, irq)

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


#define TRACE_USR_MSG(m)         TRACE_LOG(_TRC_OP_USER_MSG, m)

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

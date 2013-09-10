/*
 * system.h
 *
 *  Created on: Jul 22, 2012
 *      Author: petera
 */

#ifndef SYSTEM_H_
#define SYSTEM_H_

#include "system_config.h"
#include "system_debug.h"
#include "arch.h"
#include "shared_mem.h"

#define MIN(x,y)  ((x)<(y)?(x):(y))
#define MAX(x,y)  ((x)>(y)?(x):(y))
#define ABS(x)    ((x)<0?(-(x)):(x))

#define STR(tok) #tok

#define offsetof(st, m) \
     ((u32_t) ( (char *)&((st *)0)->m - (char *)0 ))

#ifdef CONFIG_MEMOPS
#define memcpy(d,s,n) __builtin_memcpy((d),(s),(n))
#define memset(s,t,n) __builtin_memset((s),(t),(n))
#endif

#ifdef CONFIG_VARCALL
extern void *_variadic_call(void *func, int argc, void* args);
#endif

#ifdef CONFIG_MATH
extern int _sqrt(int);
#endif

#ifdef ARCH_STM32
typedef GPIO_TypeDef * hw_io_port;
typedef uint16_t hw_io_pin;

#define GPIO_enable(port, pin) (port)->BSRR = (pin)
#define GPIO_disable(port, pin) (port)->BSRR = ((pin)<<16)
#define GPIO_set(port, pin_ena, pin_dis) (port)->BSRR = (pin_ena) | ((pin_dis)<<16)
#define GPIO_read(port, pin) (((port)->IDR & (pin)) != 0)
#endif

/**
 * Called at startup
 */
void SYS_init();
/**
 * Called from timer irq. Returns TRUE on ms update, FALSE otherwise.
 */
bool SYS_timer();
/**
 * Get milliseconds since system clock start
 */
time SYS_get_time_ms();
/**
 * Get ticks since system clock start
 */
time SYS_get_tick();
/**
 * Get current system time
 */
void SYS_get_time(u16_t *d, u8_t *h, u8_t *m, u8_t *s, u16_t *ms);
/**
 * Sets current system time
 */
void SYS_set_time(u16_t d, u8_t h, u8_t m, u8_t s, u16_t ms);

void SYS_assert(const char* file, int line);
void SYS_hardsleep_ms(u32_t ms);
u32_t SYS_build_number();
u32_t SYS_build_date();
void SYS_dump_trace(u8_t io);
void SYS_reboot(enum reboot_reason_e);

#endif /* SYSTEM_H_ */

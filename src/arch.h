/*
 * Arch definitions
 */

#ifndef _ARCH_H_
#define _ARCH_H_

#include "system.h"
#include "arch_specific.h"

void enter_critical(void);
void exit_critical(void);
bool within_critical(void);
void arch_reset(void);
void arch_sleep(void);
void arch_busywait_us(u32_t ns);

#endif /*_ARCH_H_*/

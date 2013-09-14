/*
 * Arch definitions
 */

#ifndef _ARCH_H_
#define _ARCH_H_

#include "system.h"

void enter_critical(void);
void exit_critical(void);
u32_t force_leave_critical(void);
void restore_critical(u32_t s);
void arch_reset(void);
void arch_sleep(void);

#endif /*_ARCH_H_*/

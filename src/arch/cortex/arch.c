#include "arch.h"
#include "system.h"

static volatile u32_t g_crit_entry = 0;

void enter_critical(void) {
  if ((__get_CONTROL() & 3) == 3) {
    // TODO PETER
    //print("enter critical from user!!\n");
  }
#ifndef CONFIG_ARCH_CRITICAL_DISABLE_IRQ
  __disable_irq();
#else
  CONFIG_ARCH_CRITICAL_DISABLE_IRQ;
#endif
  g_crit_entry++;
  TRACE_IRQ_OFF(g_crit_entry);
}

void exit_critical(void) {
  ASSERT(g_crit_entry > 0);
  g_crit_entry--;
  TRACE_IRQ_ON(g_crit_entry);
  if (g_crit_entry == 0) {
#ifndef CONFIG_ARCH_CRITICAL_ENABLE_IRQ
    __enable_irq();
#else
  CONFIG_ARCH_CRITICAL_ENABLE_IRQ;
#endif
  }
}

bool within_critical(void) {
  return g_crit_entry > 0;
}

void arch_reset(void) {
  NVIC_SystemReset();
}

void arch_break_if_dbg(void) {
  if (CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) {
    // if debug is enabled, fire breakpoint
    __asm__ volatile ("bkpt #0\n");
  }
}

void arch_sleep(void) {
  __WFI();
}

//void arch_busywait_us(u32_t us); // TODO PETER in proc_family now, fix

void irq_disable(void) {
  __disable_irq();
}

void irq_enable(void) {
  __enable_irq();
}



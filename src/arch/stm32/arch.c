#include "arch.h"

static u32_t g_crit_entry = 0;

void enter_critical(void) {
  if ((__get_CONTROL() & 3) == 3) {
    // TODO PETER
    //print("enter critical from user!!\n");
  }
  __disable_irq();
  g_crit_entry++;
  TRACE_IRQ_OFF(g_crit_entry);
}

void exit_critical(void) {
  ASSERT(g_crit_entry > 0);
  g_crit_entry--;
  TRACE_IRQ_ON(g_crit_entry);
  if (g_crit_entry == 0) {
    __enable_irq();
  }
}

u32_t force_leave_critical(void) {
  if (g_crit_entry > 0) {
    __enable_irq();
    return 1;
  }
  return 0;
}

void restore_critical(u32_t s) {
  if (s) {
    __disable_irq();
  }
}

void arch_reset(void) {
  NVIC_SystemReset();
}

void arch_sleep(void) {
  __WFI();
}

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

// user hardfault handler

#ifdef USER_HARDFAULT

__attribute__(( naked )) void HardFault_Handler(void) {
  asm volatile (
  "tst   lr, #4 \n\t"
  "ite   eq \n\t"
  "mrseq r0, msp \n\t"
  "mrsne r0, psp \n\t"
  "b     hard_fault_handler_c \n\t"
  );
}


#include "miniutils.h"
#include "io.h"

volatile void *stack_pointer;
volatile unsigned int stacked_r0;
volatile unsigned int stacked_r1;
volatile unsigned int stacked_r2;
volatile unsigned int stacked_r3;
volatile unsigned int stacked_r12;
volatile unsigned int stacked_lr;
volatile unsigned int stacked_pc;
volatile unsigned int stacked_psr;

__attribute__ (( weak  )) void APP_shutdown() {}

void hard_fault_handler_c(unsigned int * hardfault_args)
{
  stacked_r0 = ((unsigned long) hardfault_args[0]);
  stacked_r1 = ((unsigned long) hardfault_args[1]);
  stacked_r2 = ((unsigned long) hardfault_args[2]);
  stacked_r3 = ((unsigned long) hardfault_args[3]);

  stacked_r12 = ((unsigned long) hardfault_args[4]);
  stacked_lr = ((unsigned long) hardfault_args[5]);
  stacked_pc = ((unsigned long) hardfault_args[6]);
  stacked_psr = ((unsigned long) hardfault_args[7]);

  u32_t bfar = SCB->BFAR;
  u32_t cfsr = SCB->CFSR;
  u32_t hfsr = SCB->HFSR;
  u32_t dfsr = SCB->DFSR;
  u32_t afsr = SCB->AFSR;

  APP_shutdown();

  const u8_t io = IODBG;

  IO_blocking_tx(io, TRUE);

  IO_tx_flush(io);

  ioprint(io, TEXT_BAD("\n!!! HARDFAULT !!!\n\n"));
  ioprint(io, "Stacked registers:\n");
  ioprint(io, "  pc:   0x%08x\n", stacked_pc);
  ioprint(io, "  lr:   0x%08x\n", stacked_lr);
  ioprint(io, "  psr:  0x%08x\n", stacked_psr);
  ioprint(io, "  sp:   0x%08x\n", stack_pointer);
  ioprint(io, "  r0:   0x%08x\n", stacked_r0);
  ioprint(io, "  r1:   0x%08x\n", stacked_r1);
  ioprint(io, "  r2:   0x%08x\n", stacked_r2);
  ioprint(io, "  r3:   0x%08x\n", stacked_r3);
  ioprint(io, "  r12:  0x%08x\n", stacked_r12);
  ioprint(io, "\nFault status registers:\n");
  ioprint(io, "  BFAR: 0x%08x\n", bfar);
  ioprint(io, "  CFSR: 0x%08x\n", cfsr);
  ioprint(io, "  HFSR: 0x%08x\n", hfsr);
  ioprint(io, "  DFSR: 0x%08x\n", dfsr);
  ioprint(io, "  AFSR: 0x%08x\n", afsr);
  ioprint(io, "\n");
  if (cfsr & (1<<(7+0))) {
    ioprint(io, "MMARVALID: MemMan 0x%08x\n", SCB->MMFAR);
  }
  if (cfsr & (1<<(4+0))) {
    ioprint(io, "MSTKERR: MemMan error during stacking\n");
  }
  if (cfsr & (1<<(3+0))) {
    ioprint(io, "MUNSTKERR: MemMan error during unstacking\n");
  }
  if (cfsr & (1<<(1+0))) {
    ioprint(io, "DACCVIOL: MemMan memory access violation, data\n");
  }
  if (cfsr & (1<<(0+0))) {
    ioprint(io, "IACCVIOL: MemMan memory access violation, instr\n");
  }

  if (cfsr & (1<<(7+8))) {
    ioprint(io, "BFARVALID: BusFlt 0x%08x\n", SCB->BFAR);
  }
  if (cfsr & (1<<(4+8))) {
    ioprint(io, "STKERR: BusFlt error during stacking\n");
  }
  if (cfsr & (1<<(3+8))) {
    ioprint(io, "UNSTKERR: BusFlt error during unstacking\n");
  }
  if (cfsr & (1<<(2+8))) {
    ioprint(io, "IMPRECISERR: BusFlt error during data access\n");
  }
  if (cfsr & (1<<(1+8))) {
    ioprint(io, "PRECISERR: BusFlt error during data access\n");
  }
  if (cfsr & (1<<(0+8))) {
    ioprint(io, "IBUSERR: BusFlt bus error\n");
  }

  if (cfsr & (1<<(9+16))) {
    ioprint(io, "DIVBYZERO: UsaFlt division by zero\n");
  }
  if (cfsr & (1<<(8+16))) {
    ioprint(io, "UNALIGNED: UsaFlt unaligned access\n");
  }
  if (cfsr & (1<<(3+16))) {
    ioprint(io, "NOCP: UsaFlt execute coprocessor instr\n");
  }
  if (cfsr & (1<<(2+16))) {
    ioprint(io, "INVPC: UsaFlt general\n");
  }
  if (cfsr & (1<<(1+16))) {
    ioprint(io, "INVSTATE: UsaFlt execute ARM instr\n");
  }
  if (cfsr & (1<<(0+16))) {
    ioprint(io, "UNDEFINSTR: UsaFlt execute bad instr\n");
  }

  if (hfsr & (1<<31)) {
    ioprint(io, "DEBUGEVF: HardFlt debug event\n");
  }
  if (hfsr & (1<<30)) {
    ioprint(io, "FORCED: HardFlt SVC/BKPT within SVC\n");
  }
  if (hfsr & (1<<1)) {
    ioprint(io, "VECTBL: HardFlt vector fetch failed\n");
  }

  SYS_dump_trace(IODBG);

  SYS_break_if_dbg();

  SYS_reboot(REBOOT_CRASH);
}
#endif


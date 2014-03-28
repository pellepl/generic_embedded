/*
 * system.c
 *
 *  Created on: Jul 22, 2012
 *      Author: petera
 */

#include "system.h"
#include "system_debug.h"
#include "miniutils.h"
#include "arch.h"
#include "io.h"
#ifdef CONFIG_TASK_QUEUE
#include "taskq.h"
#endif
#ifdef CONFIG_OS
#include "os.h"
#endif

#ifdef CONFIG_CNC
#include "cnc_control.h"
#endif

volatile u32_t __dbg_mask = CONFIG_DEFAULT_DEBUG_MASK;
volatile u32_t __dbg_level = D_DEBUG;

#ifdef DBG_TRACE_MON
u16_t _trace_log[TRACE_SIZE+32]; // add slack, index incrementor is on deep waters
volatile u32_t _trace_log_ix = 0;
volatile bool __trace = TRUE;
#endif

#ifdef DBG_LEVEL_PREFIX
const char* __dbg_level_str[4] =
{ "DBG", "INF", "WRN", "FTL" };
#endif

static struct {
  volatile time time_tick;
  volatile time time_ms_c;
  volatile time time_sub;
  volatile u16_t time_ms;
  volatile u8_t time_s;
  volatile u8_t time_m;
  volatile u8_t time_h;
  volatile u16_t time_d;
} sys;

bool SYS_timer() {
  bool r = FALSE;
  sys.time_tick++;
  sys.time_sub++;
  if (sys.time_sub >= SYS_MAIN_TIMER_FREQ / SYS_TIMER_TICK_FREQ) {
    sys.time_sub = 0;
    sys.time_ms_c++;
    sys.time_ms++;
    r = TRUE;
    if (sys.time_ms == 1000) {
      sys.time_ms = 0;
      sys.time_s++;
      if (sys.time_s == 60) {
        sys.time_s = 0;
        sys.time_m++;
        if (sys.time_m == 60) {
          sys.time_m = 0;
          sys.time_h++;
          if (sys.time_h == 24) {
            sys.time_h = 0;
            sys.time_d++;
          }
        }
      }
    }
  }
  return r;
}

void SYS_init() {
  memset(&sys, 0, sizeof(sys));
#ifdef DBG_TRACE_MON
  _trace_log_ix = 0;
  memset(_trace_log, 0, sizeof(_trace_log));
#endif
}

time SYS_get_time_ms() {
  return sys.time_ms_c;
}

void SYS_get_time(u16_t *d, u8_t *h, u8_t *m, u8_t *s, u16_t *ms) {
  if (d) *d = sys.time_d;
  if (h) *h = sys.time_h;
  if (m) *m = sys.time_m;
  if (s) *s = sys.time_s;
  if (ms) *ms = sys.time_ms;
}

void SYS_set_time(u16_t d, u8_t h, u8_t m, u8_t s, u16_t ms) {
  if (h < 24 && m < 60 && s < 60 && ms < 1000) {
    sys.time_d = d;
    sys.time_h = h;
    sys.time_m = m;
    sys.time_s = s;
    sys.time_ms = ms;
  }
}

time SYS_get_tick() {
  return sys.time_tick;
}

static void (*assert_cb)(void) = NULL;

void SYS_set_assert_callback(void (*f)(void)) {
  assert_cb = f;
}

void SYS_assert(const char* file, int line) {
  irq_disable();
#ifdef CONFIG_SHARED_MEM
  SHMEM_set_reboot_reason(REBOOT_ASSERT);
#endif
  if (assert_cb) {
    assert_cb();
  }
#ifdef CONFIG_CNC
  CNC_enable_error(1<<CNC_ERROR_BIT_EMERGENCY);
#endif
  IO_blocking_tx(IODBG, TRUE);
  IO_tx_flush(IODBG);
  ioprint(IODBG, TEXT_BAD("\nASSERT: %s:%i\n"), file, line);
  IO_tx_flush(IODBG);
#ifdef CONFIG_OS
  OS_DBG_list_all(TRUE, IODBG);
  IO_tx_flush(IODBG);
#endif
#ifdef CONFIG_TASK_QUEUE
  TASK_dump(IODBG);
#endif
  IO_tx_flush(IODBG);
  SYS_dump_trace(IODBG);
  IO_tx_flush(IODBG);
  const int ASSERT_BLINK = 0x100000;
  volatile int asserted;
  __asm__ volatile ("bkpt #0\n");
  while (1) {
    asserted = ASSERT_BLINK;
    while (asserted--) {
#if 0
      if (a < ASSERT_BLINK/2) {
        GPIO_set(GPIOC, GPIO_Pin_7, GPIO_Pin_6);
      } else {
        GPIO_set(GPIOC, GPIO_Pin_6, GPIO_Pin_7);
      }
#endif
    }
  }
}

void SYS_hardsleep_ms(u32_t ms) {
  time release = SYS_get_time_ms() + ms;
  while (SYS_get_time_ms() < release);
}

void SYS_hardsleep_us(u32_t us) {
  arch_busywait_us(us);
}

void SYS_dbg_mask_set(u32_t mask) {
  __dbg_mask = mask;
}

void SYS_dbg_mask_enable(u32_t mask) {
  __dbg_mask |= mask;
}

void SYS_dbg_mask_disable(u32_t mask) {
  __dbg_mask &= ~mask;
}

void SYS_dbg_level(u32_t level) {
  __dbg_level = level;
}

u32_t SYS_dbg_get_level() {
  return __dbg_level;
}

u32_t SYS_dbg_get_mask() {
  return __dbg_mask;
}

extern char __BUILD_DATE;
extern char __BUILD_NUMBER;

u32_t SYS_build_number() {
  return (u32_t)&__BUILD_NUMBER;
}

u32_t SYS_build_date() {
  return (u32_t)&__BUILD_DATE;
}

__attribute__ (( noreturn )) void SYS_reboot(enum reboot_reason_e r) {
  arch_reset();
  while (1);
}

#ifdef DBG_TRACE_MON
bool __trace_start(void) {
  bool o = __trace;
  __trace = TRUE;
  return o;
}
bool __trace_stop(void) {
  bool o = __trace;
  __trace = FALSE;
  return o;
}
bool __trace_set(bool x) {
  bool o = __trace;
  __trace = x;
  return o;
}
#endif

void SYS_dump_trace(u8_t io) {
#ifdef DBG_TRACE_MON
  bool tracing = TRACE_STOP();

#ifdef CONFIG_OS
  char cur_thread[16];
  memset(cur_thread, '.', sizeof(cur_thread));
  cur_thread[sizeof(cur_thread)-1] = 0;
#else
  char cur_thread[] = " ";
#endif

  bool old_blocking_tx = IO_blocking_tx(io, TRUE);
  IO_tx_flush(io);
  u32_t s_ix = _trace_log_ix;
  if (s_ix == 0) {
    s_ix = TRACE_SIZE - 1;
  } else {
    s_ix--;
  }
  u32_t ix = s_ix;
  u32_t t_len = 0;
  // find trace start
  while (_trace_log[ix] != 0) {
    t_len++;
    if (ix == 0) {
      ix = TRACE_SIZE - 1;
    } else {
      ix--;
    }
    if (ix == s_ix) break;
  }
  // display all trace chronologically
  while (t_len-- > 0)  {
    if (ix == TRACE_SIZE - 1) {
      ix = 0;
    } else {
      ix++;
    }
    u16_t e = _trace_log[ix];
    _trc_types op = e >> 8;
    u8_t arg = e & 0xff;
    if (_trace_log[ix] == 0) {
      break;
    }
#ifdef CONFIG_OS
    os_thread *t;
#endif
    switch (op) {
#ifdef CONFIG_OS
    case _TRC_OP_OS_CTX_LEAVE:
    case _TRC_OP_OS_CTX_ENTER:
    case _TRC_OP_OS_CREATE:
    case _TRC_OP_OS_YIELD:
    case _TRC_OP_OS_THRSLEEP:
    case _TRC_OP_OS_MUTACQ_LOCK:
    case _TRC_OP_OS_MUTWAIT_LOCK:
    case _TRC_OP_OS_SIGWAKED:
    case _TRC_OP_OS_THRWAKED:
    case _TRC_OP_OS_DEAD:
      t = OS_DBG_get_thread_by_id(arg);
      if (t != NULL) {
        if (op == _TRC_OP_OS_CTX_ENTER) {
          // use new thread name as prefix
          if (t->name) {
            strncpy(cur_thread, t->name, sizeof(cur_thread)-2);
          } else {
            itoan(t->id, cur_thread, 10, sizeof(cur_thread)-2);
          }
          int l = strlen(cur_thread);
          memset(cur_thread + l, ' ', (sizeof(cur_thread)-1) - l);
        } else if (op == _TRC_OP_OS_CTX_LEAVE) {
          memset(cur_thread, ' ', sizeof(cur_thread)-1);
        }
        ioprint(io, "%s  %s  %s\n", cur_thread, TRACE_NAMES[op], t->name);
      } else {
        if (op == _TRC_OP_OS_CTX_ENTER) {
          memset(cur_thread, '?', sizeof(cur_thread)-1);
        } else if (op == _TRC_OP_OS_CTX_LEAVE) {
          memset(cur_thread, ' ', sizeof(cur_thread)-1);
        }
        ioprint(io, TEXT_BAD("%s  %s  unknown thread id: %02x\n"), cur_thread, TRACE_NAMES[op], arg);
      }
      break;
#endif
    case _TRC_OP_IRQ_ENTER:
    case _TRC_OP_IRQ_EXIT:
      ioprint(io, "%s  %s  %s\n", cur_thread, TRACE_NAMES[op], TRACE_IRQ_NAMES[arg]);
      break;
#ifdef CONFIG_OS
    case _TRC_OP_OS_SLEEP:
      memset(cur_thread, ' ', sizeof(cur_thread)-1);
      // fall through
      //no break
#endif
    default:
      ioprint(io, "%s  %s  %02x\n", cur_thread, TRACE_NAMES[op], arg);
      break;
    }
  };
  IO_tx_flush(io);

  IO_blocking_tx(io, old_blocking_tx);
  TRACE_SET(tracing);
#endif
}


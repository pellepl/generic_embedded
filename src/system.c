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
#if defined(CONFIG_RTC) && defined(CONFIG_SYS_USE_RTC)
#include "rtc.h"
#endif
#ifdef CONFIG_SHARED_MEM
#include "shared_mem.h"
#endif

#ifndef DBG_ATTRIBUTE
#define DBG_ATTRIBUTE
#endif

DBG_ATTRIBUTE volatile u32_t __dbg_mask = CONFIG_DEFAULT_DEBUG_MASK;
DBG_ATTRIBUTE volatile u32_t __dbg_level = D_DEBUG;

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
#if defined(CONFIG_RTC) && defined(CONFIG_SYS_USE_RTC)
#else
  volatile sys_time time_tick;
  volatile sys_time time_ms_c;
  volatile sys_time time_sub;
  volatile u16_t time_ms;
  volatile u8_t time_s;
  volatile u8_t time_m;
  volatile u8_t time_h;
  volatile u16_t time_d;
#endif
} sys;

bool SYS_timer() {
  bool r = FALSE;
#if defined(CONFIG_RTC) && defined(CONFIG_SYS_USE_RTC)
  static u64_t pre_rtc_tick = 0;
  u64_t rtc_tick = RTC_get_tick();
  r = RTC_TICK_TO_MS(pre_rtc_tick) != RTC_TICK_TO_MS(rtc_tick);
  pre_rtc_tick = rtc_tick;
#else
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
#endif
  return r;
}

void SYS_init() {
  memset(&sys, 0, sizeof(sys));
#ifdef DBG_TRACE_MON
  _trace_log_ix = 0;
  memset(_trace_log, 0, sizeof(_trace_log));
#endif
}

sys_time SYS_get_time_ms() {
#if defined(CONFIG_RTC) && defined(CONFIG_SYS_USE_RTC)
  u64_t t = RTC_get_tick();
  return (sys_time)(RTC_TICK_TO_MS(t));
#else
  return sys.time_ms_c;
#endif
}

void SYS_get_time(u16_t *d, u8_t *h, u8_t *m, u8_t *s, u16_t *ms) {
#if defined(CONFIG_RTC) && defined(CONFIG_SYS_USE_RTC)
  rtc_datetime dt;
  RTC_get_date_time(&dt);
  if (d) *d = dt.date.year_day;
  if (h) *h = dt.time.hour;
  if (m) *h = dt.time.minute;
  if (ms) *h = dt.time.millisecond;
#else
  if (d) *d = sys.time_d;
  if (h) *h = sys.time_h;
  if (m) *m = sys.time_m;
  if (s) *s = sys.time_s;
  if (ms) *ms = sys.time_ms;
#endif
}

void SYS_set_time(u16_t d, u8_t h, u8_t m, u8_t s, u16_t ms) {
  if (h < 24 && m < 60 && s < 60 && ms < 1000) {
#if defined(CONFIG_RTC) && defined(CONFIG_SYS_USE_RTC)
    rtc_datetime dt;
    RTC_get_date_time(&dt);
    u64_t dt_sec = RTC_datetime2secs(&dt);
    dt_sec += (d - dt.date.year_day) * 24ULL * 60ULL * 60ULL +
              (h - dt.time.hour) * 60ULL * 60ULL +
              (m - dt.time.minute) * 60ULL +
              (s - dt.time.second);
    RTC_secs2datetime(dt_sec, &dt);
    RTC_set_date_time(&dt);
#else
    sys.time_d = d;
    sys.time_h = h;
    sys.time_m = m;
    sys.time_s = s;
    sys.time_ms = ms;
#endif
  }
}

sys_time SYS_get_tick() {
#if defined(CONFIG_RTC) && defined(CONFIG_SYS_USE_RTC)
  return RTC_get_tick();
#else
  return sys.time_tick;
#endif
}

static void (*assert_cb)(void) = NULL;
static assert_behaviour_t assert_behaviour = ASSERT_BLINK_4EVER;

void SYS_set_assert_callback(void (*f)(void)) {
  assert_cb = f;
}

void SYS_set_assert_behaviour(assert_behaviour_t b) {
  assert_behaviour = b;
}

void SYS_break_if_dbg(void) {
  arch_break_if_dbg();
}

void SYS_assert(const char* file, int line) {
  irq_disable();
  if (assert_cb) {
    assert_cb();
  }

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

  SYS_break_if_dbg();

  if (assert_behaviour == ASSERT_RESET) {
    SYS_reboot(REBOOT_ASSERT);
  }

  const int ASSERT_BLINK = 0x100000;
  volatile int asserted;
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
#if defined(CONFIG_RTC) && defined(CONFIG_SYS_USE_RTC)
  u64_t tick_release = RTC_get_tick() + RTC_MS_TO_TICK((u64_t)ms);
  while (RTC_get_tick() < tick_release);
#else
  time release = SYS_get_time_ms() + ms;
  while (SYS_get_time_ms() < release);
#endif
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
#ifdef CONFIG_SHARED_MEM
  SHMEM_set_reboot_reason(r);
#endif
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


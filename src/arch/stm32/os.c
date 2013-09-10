#include "os.h"
#include "miniutils.h"
#include "list.h"
#include "linker_symaccess.h"

#define OS_THREAD_FLAG_ALIVE        (1<<0)
#define OS_THREAD_FLAG_SLEEP        (1<<1)
#define OS_SVC_YIELD                (0x00010000)
#define OS_FOREVER                  ((time)-1)
#define OS_STACK_START_MARKER       (0xf00dcafe)
#define OS_STACK_END_MARKER         (0xfadebeef)

#define OS_ELEMENT(obj) &((obj)->this.e)
#define OS_TYPE(obj) ((obj)->type)
#define OS_OBJ(ele) ((os_object *)((char *)(ele) + ((char *)&((os_object *)0)->e - (char *)0 )))
#define OS_THREAD(ele) ((os_thread *)((char *)(ele) + ((char *)&((os_thread *)0)->this.e - (char *)0 )))
#define OS_COND(ele) ((os_cond *)((char *)(ele) + ((char *)&((os_cond *)0)->this.e - (char *)0 )))

#define _STACK_USAGE_MARK (0xea)
#define _STACK_USAGE_MARK_32 ((_STACK_USAGE_MARK << 24) | (_STACK_USAGE_MARK << 16) | (_STACK_USAGE_MARK << 8) | _STACK_USAGE_MARK)

#define PENDING_CTX_SWITCH SCB->ICSR |= SCB_ICSR_PENDSVSET

typedef struct {
  u32_t r0;
  u32_t r1;
  u32_t r2;
  u32_t r3;
  u32_t r12;
  u32_t lr;
  u32_t pc;
  u32_t psr;
} cortex_frame;

typedef struct {
  u32_t r4;
  u32_t r5;
  u32_t r6;
  u32_t r7;
  u32_t r8;
  u32_t r9;
  u32_t r10;
  u32_t r11;
} thread_frame;

#define OS_CANARY_MAGIC 0xf0f0feed

static struct os {
#if OS_RUNTIME_VALIDITY_CHECK
  u32_t os_canary_pre;
#endif
  // pointer to current thread
  os_thread *current_thread;
  // flags of current thread
  u32_t current_flags;
#if CONFIG_OS_BUMP
  // pointer to bumped thread
  os_thread *bumped_thread;
#endif
  // msp on first context switch
  void *main_msp;
  // thread running queue
  list_t q_running;
  // thread sleeping queue
  list_t q_sleep;
  time first_awake;
  volatile bool preemption;
#if OS_RUNTIME_VALIDITY_CHECK
  u32_t os_canary_post;
#endif
#if OS_DBG_MON
  u8_t thread_peer_ix;
  os_thread *thread_peers[OS_THREAD_PEERS];
  u8_t mutex_peer_ix;
  os_mutex *mutex_peers[OS_MUTEX_PEERS];
  u8_t cond_peer_ix;
  os_cond *cond_peers[OS_COND_PEERS];
#endif
} os;

static volatile u32_t g_thr_id = 0;
static volatile u32_t g_mutex_id = 0;
static volatile u32_t g_cond_id = 0;
static volatile u8_t g_crit_entry = 0;

u32_t __os_ctx_switch(void *sp);
static void __os_thread_death(void);
static void __os_update_preemption();
static void __os_disable_preemption(void);
static void __os_enable_preemption(void);

//------------ debug checks -------------
static void __os_check_validity() {
#if OS_RUNTIME_VALIDITY_CHECK
  // check struct
  ASSERT(os.os_canary_pre == OS_CANARY_MAGIC);
  ASSERT(os.os_canary_post == OS_CANARY_MAGIC);
  // check queue
  {
    element_t *e;
    e = list_first(&os.q_running);
    while (e) {
      os_type type = OS_TYPE(OS_OBJ(e));
      ASSERT(type == OS_THREAD);
      e = list_next(e);
    }
  }
#endif
}


//------------ irq calls -------------

// called from SVC_Handler in svc_handler.s
void __os_svc_handler(u32_t *args) {
  u32_t svc_nbr;
  u32_t arg0;
  u32_t arg1;
  u32_t arg2;
  u32_t arg3;

  svc_nbr = ((u8_t *)args[6])[-2]; // get stacked_pc - 2
  arg0 = args[0];
  arg1 = args[1];
  arg2 = args[2];
  arg3 = args[3];

  //print("SVC call #%i %08x %08x %08x %08x\n", svc_nbr, arg0, arg1, arg2, arg3);

  if (svc_nbr == 1) {
// TODO
//    if (arg0 == OS_SVC_YIELD) {
      PENDING_CTX_SWITCH;
      return;
//    }
  }
  ASSERT(FALSE);
}

// Called from SysTick_Handler in stm32f103x_it.c
void __os_systick(void) {
  // assert pendsv
  PENDING_CTX_SWITCH;
}

// Called from PendSV_Handler in stm32f103x_it.c
// Avoid nesting: cannot be a nested interrupt, as it is pendsv
// with lowest prio.
// Not in c, we can never know what compiler does with registers
// and stack after restoring context upon function exit.
__attribute__((naked)) void __os_pendsv(void)  {
  asm volatile (
      // if os.current_flags != 0, then store context
      "ldr     r2, __os\n"
      "ldr     r1, [r2, %0]\n"    // get os.current_flags
      "cbz     r1, __no_leave_context\n"
      // ....store thread context
      "mrs     r0, psp\n"
      "stmdb   r0!, {r4-r11}\n"
      "msr     psp, r0\n"

      // call __os_ctx_switch
      "__do_ctx_switch:\n"
      // takes r0 = current sp
      "bl      __os_ctx_switch\n"
      // returns r0 = flags of new thread or 0
      //         r1 = new sp

      // if got new thread
      "cbz     r0, __no_enter_context\n"

      // ..restore context, set user or privileged
      "mrs     r2, CONTROL\n"
      "tst     r0, #(1<<2)\n"     // is OS_THREAD_FLAG_PRIVILEGED set?
      "ite     ne\n"
      "andne   r2, #1\n"      // privileged
      "orreq   r2, #1\n"      // user
      "msr     CONTROL, r2\n"
      // ....restore thread context
      "ldmfd   r1!, {r4-r11}\n"
      "msr     psp, r1\n"
      "mvn     lr, #2\n" // __THREAD_RETURN
      "bx      lr\n"

      // ..else if no new thread, get ass back to kernel main
      "__no_enter_context:\n"
      "ldr     r2, __os\n"
      "ldr     r1, [r2, %1]\n"  // r1 = os.main_msp
      "str     r0, [r2, %1]\n"  // os.current_flags = 0
      "msr     msp, r1\n"       // msp = r1
      "pop     {r4-r11}\n"
      "mvn     lr, #6\n" // __MAIN_RETURN, kernel
      "bx      lr\n"
      // .. store kernel main context
      "__no_leave_context:\n"
      "ldr     r1, [r2, %1]\n"    // get os.main_msp
      "teq     r1, #0\n"          // if zero, first ctx switch ever
      "ittt    eq\n"
      "pusheq  {r4-r11}\n"
      "mrseq   r0, msp\n"
      "streq   r0, [r2, %1]\n"    // store os.main_msp
      "b       __do_ctx_switch\n"
      ""
      "__os:"
      ".word   os"
      :
      : "n"(offsetof(struct os, current_flags)), "n"(offsetof(struct os, main_msp))
  );
}

//------- System helpers -------------

u32_t __os_ctx_switch(void *sp) {
  os_thread *cand = NULL;
  __CLREX();  // removes the local exclusive access tag for the processor

  enter_critical();

  if (os.current_thread != NULL) {
    TRACE_OS_CTX_LEAVE(os.current_thread);
    // save current psp to current thread
    if (os.current_thread->flags & OS_THREAD_FLAG_ALIVE) {
      os.current_thread->sp = sp;
    }
#if OS_STACK_CHECK
    ASSERT(*(u32_t*)(os.current_thread->stack_start - 4) == OS_STACK_START_MARKER);
    ASSERT(*(u32_t*)(os.current_thread->stack_end) == OS_STACK_END_MARKER);
#endif
  }

  // find next candidate
#if CONFIG_OS_BUMP
  if (os.bumped_thread != NULL) {
    // if we have a bumped thread, prefer that to anything else
    cand = os.bumped_thread;
    os.bumped_thread = NULL;
  } else
#endif
  {
    cand = OS_THREAD(list_first(&os.q_running));
  }

  // reorder prio
  if (cand != NULL) {
    list_move_last(&os.q_running, OS_ELEMENT(cand));
  }
  exit_critical();

  if (cand != NULL) {
    // got a candidate, setup context
    os.current_flags = cand->flags;
    TRACE_OS_CTX_ENTER(cand);
  } else {
    // no candidate, goto sleep
    os.current_flags = 0;
    TRACE_OS_SLEEP(list_count(&os.q_running));
  }
  os.current_thread = cand;
  __os_update_preemption();


  asm volatile ("nop"); // compiler reorder barrier
  if (os.current_flags) {
    // move candidate's sp to R1 before return
    asm volatile (
        "mov    r1, %0\n"
        :
        : "r"(cand->sp)
        : "r1"
    );
  }
  asm volatile ("nop"); // compiler reorder barrier
  return os.current_flags;
}

static void __os_sleepers_update(list_t *q, time now) {
  element_t *e;
  e = list_first(q);
  while (e && list_get_order(e) <= now) {
    // get sleeper element
    os_type type = OS_TYPE(OS_OBJ(e));
    enter_critical();
    element_t *next_e = list_next(e);
    exit_critical();

    switch (type) {
    case OS_THREAD: {
      // it was a thread, simply move from sleeping to running
      os_thread *t = OS_THREAD(e);
      enter_critical();
      TRACE_OS_THRWAKED(t);
      list_delete(q, e);
      list_add(&os.q_running, e);
      list_set_order(e, OS_FOREVER);
      t->flags &= ~OS_THREAD_FLAG_SLEEP;
      t->ret_val = TRUE;
      __os_check_validity();
      exit_critical();
    }
    break;

    case OS_COND: {
      // it was a conditional, recurse into conditional's sleep queue
      os_cond *c = OS_COND(e);
      if (now >= e->sort_order) {
        TRACE_OS_CONDTIMWAKED(c);
        __os_sleepers_update(&c->q_sleep, now);
        enter_critical();
        if (list_is_empty(&c->q_sleep)) {
          // conds sleep queue got empty, remove from os's sleep queue
          list_set_order(c, OS_FOREVER);
          c->has_sleepers = FALSE;
          list_delete(&os.q_sleep, e);
        } else {
          // conds sleep queue not empty, update in os's sleep queue
          list_set_order(c, list_get_order(list_first(&c->q_sleep)));
          list_delete(&os.q_sleep, e);
          list_sort_insert(&os.q_sleep, e);
        }
        __os_check_validity();
        exit_critical();
      }
    }
    break;
    }
    e = next_e;
  }
}

static void __os_update_first_awake() {
  if (list_is_empty(&os.q_sleep)) {
    os.first_awake = OS_FOREVER;
  } else {
    os.first_awake = list_get_order(list_first(&os.q_sleep));
  }
}

static void __os_update_preemption() {
  if (os.q_running.length <= 1) {
    __os_disable_preemption();
  } else {
    __os_enable_preemption();
  }
}

void __os_time_tick(time now) {
  if (now >= os.first_awake) {
    __os_sleepers_update(&os.q_sleep, now);
    __os_update_first_awake();
    if (!os.preemption) {
      PENDING_CTX_SWITCH;
    }
    __os_update_preemption();
  }
}

__attribute__((noreturn)) static void __os_thread_death(void) {
  enter_critical();
  TRACE_OS_THRDEAD(os.current_thread);
  list_delete(&os.q_running, OS_ELEMENT(os.current_thread));
  list_move_all(&os.q_running, &os.current_thread->q_join);
  os.current_thread->flags = 0;
  os.current_thread = NULL;
  __os_check_validity();
  exit_critical();
  (void)OS_thread_yield();
  // will never execute this
  while (1) {
    ASSERT(FALSE);
  }
}

// disable systick
static inline void __os_disable_preemption(void) {
  if (os.preemption) {
    TRACE_OS_PREEMPTION(0);
  }
  os.preemption = FALSE;
  SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
}

// enable systick
static inline void __os_enable_preemption(void) {
  if (!os.preemption) {
    TRACE_OS_PREEMPTION(1);
  }
  os.preemption = TRUE;
  SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
}

static bool OS_enter_critical() {
  bool pre = os.preemption;
  __os_disable_preemption();
  return pre;
}

static void OS_exit_critical(bool pre) {
  if (pre) {
    __os_enable_preemption();
  }
}

//------- Public functions -----------

u32_t OS_thread_create(os_thread *t, u32_t flags, void *(*func)(void *), void *arg, void *stack, u32_t stack_size, const char *name) {
  // align stack pointer
  if ((u32_t)stack & 0x3) {
    stack = (void *)((u32_t)stack + (4-((u32_t)stack & 0x3)));
    stack_size -= (4-((u32_t)stack & 0x3));
  }
  stack_size &= ~3;

#if OS_STACK_CHECK
  *(u32_t *)stack = OS_STACK_START_MARKER;
  *(u32_t *)(stack + stack_size - 4) = OS_STACK_END_MARKER;
  stack += 4;
  stack_size -= 4*2;
#endif

#if OS_STACK_USAGE_CHECK
  memset(stack, _STACK_USAGE_MARK, stack_size);
#endif

  // create startup frame
  cortex_frame *init_frame = (cortex_frame *)(stack + stack_size - sizeof(cortex_frame));
  init_frame->r0 = (u32_t)arg;
  init_frame->r1 = 0;
  init_frame->r2 = 0;
  init_frame->r3 = 0;
  init_frame->r12 = 0;
  init_frame->pc = (u32_t)func;
  init_frame->lr = (u32_t)__os_thread_death;
  init_frame->psr = 0x21000000; // default psr

  t->flags = OS_THREAD_FLAG_ALIVE | flags;
  t->sp = stack + stack_size - sizeof(cortex_frame) - sizeof(thread_frame);
  t->stack_start = stack;
  t->stack_end = (void*)(stack + stack_size);
  t->func = func;
  t->prio = 128;

  t->this.type = OS_THREAD;

  t->id = ++g_thr_id;
  t->name = name;

  list_init(&t->q_join);
  list_set_order(OS_ELEMENT(t), OS_FOREVER);

  enter_critical();
  list_add(&os.q_running, OS_ELEMENT(t));
#if OS_DBG_MON & OS_THREAD_PEERS > 0
  {
    int i;
    for (i = 0; i < OS_THREAD_PEERS; i++) {
      if (os.thread_peers[i] == t) {
        break;
      }
    }
    if (i == OS_THREAD_PEERS) {
      os.thread_peers[os.thread_peer_ix++] = t;
      if (os.thread_peer_ix >= OS_THREAD_PEERS) {
        os.thread_peer_ix = 0;
      }
    }
  }
#endif
  TRACE_OS_THRCREATE(t);
  __os_check_validity();
  exit_critical();

  __os_enable_preemption();

  return 0;
}

os_thread *OS_thread_self() {
  return os.current_thread;
}

u32_t OS_thread_self_id() {
  return os.current_thread->id;
}

u32_t OS_thread_id(os_thread *t) {
  return t->id;
}

u32_t OS_thread_yield(void) {
  ASSERT(g_crit_entry == 0);
  TRACE_OS_YIELD(OS_thread_self());
  OS_svc_1((void*)OS_SVC_YIELD,0,0,0);
  return OS_thread_self()->ret_val;
}

void OS_thread_join(os_thread *t) {
  os_thread *self = OS_thread_self();
  ASSERT(self);
  ASSERT(t != self);
  enter_critical();
  if (t->flags & OS_THREAD_FLAG_ALIVE) {
    list_delete(&os.q_running, OS_ELEMENT(self));
    list_add(&t->q_join, OS_ELEMENT(self));
  }
  exit_critical();
}

u32_t OS_mutex_init(os_mutex *m, u32_t attrs) {
  m->id = ++g_mutex_id;
  m->lock = 0;
  m->owner = 0;
  m->attrs = attrs;
  list_init(&m->q_block);
#if OS_DBG_MON & OS_MUTEX_PEERS > 0
  os.mutex_peers[os.mutex_peer_ix++] = m;
  if (os.mutex_peer_ix >= OS_MUTEX_PEERS) {
    os.mutex_peer_ix = 0;
  }
#endif
  return 0;
}

void OS_thread_sleep(time delay) {
  os_thread *self = OS_thread_self();
  time awake = SYS_get_time_ms() + delay;
  enter_critical();
  TRACE_OS_THRSLEEP(self);
  list_delete(&os.q_running, OS_ELEMENT(self));
  list_set_order(OS_ELEMENT(self), awake);
  list_sort_insert(&os.q_sleep, OS_ELEMENT(self));
  __os_update_first_awake();
  self->flags |= OS_THREAD_FLAG_SLEEP;
  exit_critical();
  (void)OS_thread_yield();
}

static u32_t OS_mutex_lock_internal(os_mutex *m) {
  os_thread *self = OS_thread_self();
  bool taken = TRUE;
  u32_t res = 0;

  if (m->attrs & OS_MUTEX_ATTR_REENTRANT) {
    enter_critical();
    if (m->owner == self) {
      m->depth++;
      exit_critical();
      return 0;
    }
    exit_critical();
  } else {
    ASSERT(m->owner != self);
  }

  TRACE_OS_MUTLOCK(m);
  do {
    taken = TRUE;
    u32_t v;
    // try setting mutex flag to busy
    do {
      v =  __LDREXW(&m->lock);
      if (v) {
        __CLREX();  // removes the local exclusive access tag for the processor
        taken = FALSE;
        break;
      }
    } while (__STREXW(1, &m->lock));

    if (!taken) {
      // mutex already busy
      enter_critical();
      TRACE_OS_MUT_WAITLOCK(self);
      list_delete(&os.q_running, OS_ELEMENT(self));
      list_add(&m->q_block, OS_ELEMENT(self));
      exit_critical();
      res = OS_thread_yield();
    } else {
      // mutex taken
      enter_critical();
      TRACE_OS_MUT_ACQLOCK(self);
      m->owner = self;
      if (m->attrs & OS_MUTEX_ATTR_REENTRANT) {
        m->depth = 1;
      }

#if OS_DBG_MON
      m->entered++;
#endif
      if ((m->attrs & OS_MUTEX_ATTR_CRITICAL_IRQ) == 0) {
        // mutex is irq safe, keep critical lock gained when taking mutex
        exit_critical();
      }

    }
  } while (!taken);
  return res;
}

u32_t OS_mutex_lock(os_mutex *m) {
  u32_t r;
  r = OS_mutex_lock_internal(m);
  return r;
}

u32_t OS_mutex_unlock_internal(os_mutex *m, bool full) {
  ASSERT(m->owner == OS_thread_self());

  // reinsert all waiting threads to running queue
  enter_critical();
  if (m->attrs & OS_MUTEX_ATTR_REENTRANT) {
    if (full) {
      // full reentrant unlock
      m->depth = 0;
    }
    if (m->depth > 0) {
      m->depth--;
      exit_critical();
      return m->depth + 1;
    }
  }

  TRACE_OS_MUTUNLOCK(m);
  list_move_all(&os.q_running, &m->q_block);
  // reset mutex
  m->lock = 0;

  m->owner = 0;
#if OS_DBG_MON
      m->exited++;
#endif
  __os_check_validity();
  __os_update_preemption();
  exit_critical();
  if (m->attrs & OS_MUTEX_ATTR_CRITICAL_IRQ) {
    // mutex was irq safe, release the extra critical lock taken in mutex_lock
    exit_critical();
  }


  return 0;
}

u32_t OS_mutex_unlock(os_mutex *m) {
  u32_t r;
  r = OS_mutex_unlock_internal(m, FALSE);
  return r;
}

bool OS_mutex_try_lock(os_mutex *m) {
  os_thread *self = OS_thread_self();
  bool taken = TRUE;

  if (m->attrs & OS_MUTEX_ATTR_REENTRANT) {
    enter_critical();
    if (m->owner == self) {
      m->depth++;
      exit_critical();
      return TRUE;
    }
    exit_critical();
  } else {
    ASSERT(m->owner != self);
  }

  TRACE_OS_MUTLOCK(m);

  // try setting mutex flag to busy
  do {
    u32_t v;
    v =  __LDREXW(&m->lock);
    if (v) {
      __CLREX();  // removes the local exclusive access tag for the processor
      taken = FALSE;
      break;
    }
  } while (__STREXW(1, &m->lock));

  if (!taken) {
    // mutex already busy
    return FALSE;
  }

  TRACE_OS_MUT_ACQLOCK(m);
  m->owner = self;
  if (m->attrs & OS_MUTEX_ATTR_REENTRANT) {
    m->depth = 1;
  }

  return TRUE;
}

u32_t OS_cond_init(os_cond *c) {
  c->id = ++g_cond_id;
  c->this.type = OS_COND;
  list_set_order(OS_ELEMENT(c), OS_FOREVER);
  list_init(&c->q_block);
  list_init(&c->q_sleep);
#if OS_DBG_MON
  c->waiting = 0;
  c->signalled = 0;
  c->broadcasted = 0;
  os.cond_peers[os.cond_peer_ix++] = c;
  if (os.cond_peer_ix >= OS_COND_PEERS) {
    os.cond_peer_ix = 0;
  }
#endif
  return 0;
}

u32_t OS_cond_wait(os_cond *c, os_mutex *m) {
  os_thread *self = OS_thread_self();
  u32_t r;
  ASSERT((void*)c >= RAM_BEGIN);
  ASSERT((void*)c < RAM_END);
  enter_critical();
  TRACE_OS_CONDWAIT(c);
  //ASSERT(strcmp("main_kernel", self->name) != 0);
  if (m) {
    (void)OS_mutex_unlock_internal(m, TRUE);
  }
  list_delete(&os.q_running, OS_ELEMENT(self));
  list_add(&c->q_block, OS_ELEMENT(self));
  c->mutex = m;
#if OS_DBG_MON
  c->waiting++;
#endif
  exit_critical();
  r = OS_thread_yield();
  if (m) {
    (void)OS_mutex_lock_internal(m);
  }
  return r;
}

u32_t OS_cond_timed_wait(os_cond *c, os_mutex *m, time delay) {
  bool into_sleep_queue;
  u32_t r;
  os_thread *self = OS_thread_self();
  self->ret_val = FALSE;
  enter_critical();

  ASSERT(strcmp("main_kernel", self->name) != 0);

  TRACE_OS_CONDTIMWAIT(c);

  if (m) {
    (void)OS_mutex_unlock_internal(m, TRUE);
  }

  into_sleep_queue = c->has_sleepers;
  time awake = SYS_get_time_ms() + delay;
  list_delete(&os.q_running, OS_ELEMENT(self));
  list_set_order(OS_ELEMENT(self), awake);
  list_sort_insert(&c->q_sleep, OS_ELEMENT(self));

  c->has_sleepers = TRUE;

  if (list_get_order(OS_ELEMENT(c)) > awake) {
    list_set_order(OS_ELEMENT(c), awake);
  }
  if (into_sleep_queue) {
    // remove first so placement is updated
    list_delete(&os.q_sleep, OS_ELEMENT(c));
  }
  // insert into sleep queue
  list_sort_insert(&os.q_sleep, OS_ELEMENT(c));

  os.first_awake = MIN(awake, os.first_awake);
  c->mutex = m;
#if OS_DBG_MON
  c->waiting++;
#endif
  exit_critical();
  r = OS_thread_yield();
  if (m) {
    (void)OS_mutex_lock_internal(m);
  }
  return r;
}


u32_t OS_cond_signal(os_cond *c) {
  os_thread *t;
  enter_critical();
  TRACE_OS_CONDSIG(c);

  // first, check if there are sleepers
  if (!list_is_empty(&c->q_sleep)) {

    // wake up first timed waiter
    t = OS_THREAD(list_first(&c->q_sleep));
    TRACE_OS_SIGWAKED(t);
    if (os.current_thread != t) {
#if CONFIG_OS_BUMP
      // play it nice and do not bump if thread is already running
      os.bumped_thread = t;
#endif
    }
    list_delete(&c->q_sleep, OS_ELEMENT(t));
    // did the condition's sleep queue become empty?
    if (list_is_empty(&c->q_sleep)) {
      c->has_sleepers = FALSE;
      // yep, remove condition from os sleep queue and update first_awake value.
      list_delete(&os.q_sleep, OS_ELEMENT(c));
      __os_update_first_awake();
    }
  } else {
  // no sleepers, wake first blockee
    t = OS_THREAD(list_first(&c->q_block));
    if (t != NULL) {
      TRACE_OS_SIGWAKED(t);
      if (os.current_thread != t) {
#if CONFIG_OS_BUMP
        // play it nice and do not bump if thread is already running
        os.bumped_thread = t;
#endif
      }
      list_delete(&c->q_block, OS_ELEMENT(t));
    }
  }
  if (t != NULL) {
    list_add(&os.q_running, OS_ELEMENT(t));
#if OS_DBG_MON
  c->signalled++;
#endif
  }
  __os_check_validity();
  __os_update_preemption();
  exit_critical();
  if (!os.preemption) {
    PENDING_CTX_SWITCH;
  }

  return 0;
}

u32_t OS_cond_broadcast(os_cond *c) {
  enter_critical();
  TRACE_OS_CONDBROAD(c);

  // wake all sleepers
  if (!list_is_empty(&c->q_sleep)) {
    if (os.current_thread != OS_THREAD(list_first(&c->q_sleep))) {
#if CONFIG_OS_BUMP
      // play it nice and do not bump if thread is already running
      os.bumped_thread = OS_THREAD(list_first(&c->q_sleep));
#endif
    }
    TRACE_OS_SIGWAKED(OS_THREAD(list_first(&c->q_sleep)));
    list_move_all(&os.q_running, &c->q_sleep);
    //  remove condition from os sleep queue and update first_awake value.
    c->has_sleepers = FALSE;
    list_delete(&os.q_sleep, OS_ELEMENT(c));
    __os_update_first_awake();
  } else {
    // if no sleepers, bump first blocked thread
    if (!list_is_empty(&c->q_block)) {
      if (os.current_thread != OS_THREAD(list_first(&c->q_block))) {
#if CONFIG_OS_BUMP
        // play it nice and do not bump if thread is already running
        os.bumped_thread = OS_THREAD(list_first(&c->q_block));
#endif
      }
      TRACE_OS_SIGWAKED(OS_THREAD(list_first(&c->q_block)));
    }
  }
  // wake all blockees
  list_move_all(&os.q_running, &c->q_block);
#if OS_DBG_MON
  c->broadcasted++;
#endif
  __os_check_validity();
  __os_update_preemption();
  exit_critical();
  if (!os.preemption) {
    PENDING_CTX_SWITCH;
  }
  return 0;
}


void OS_svc_0(void *arg, ...) {
  asm volatile (
      "svc 0\n"
  );
}

void OS_svc_1(void *arg, ...) {
  asm volatile (
      "svc 1\n"
  );
}

void OS_svc_2(void *arg, ...) {
  asm volatile (
      "svc 2\n"
  );
}

void OS_svc_3(void *arg, ...) {
  asm volatile (
      "svc 3\n"
  );
}

void OS_init(void) {
  const u32_t ticks = (SystemCoreClock/8) / CONFIG_OS_PREEMPT_FREQ;
  memset(&os, 0, sizeof(os));

#if OS_RUNTIME_VALIDITY_CHECK
  os.os_canary_pre = OS_CANARY_MAGIC;
  os.os_canary_post = OS_CANARY_MAGIC;
#endif

  list_init(&os.q_running);
  list_init(&os.q_sleep);
  os.first_awake = OS_FOREVER;

  SysTick->LOAD  = (ticks & SysTick_LOAD_RELOAD_Msk) - 1; // set reload reg
  SysTick->VAL   = 0;
  SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk;
  SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8);
}

#if OS_DBG_MON
static void OS_DBG_print_thread_list(u8_t io, list_t *l, bool detail, int indent);

bool OS_DBG_print_thread(u8_t io, os_thread *t, bool detail, int indent) {
  if (t == NULL) return FALSE;
  char tab[32];
  memset(tab, ' ', sizeof(tab));
  tab[indent] = 0;
  ioprint(io, "%sthread id:%04x  addr:%08x  name:%s  order:%08x\n", tab,
      t->id, t, t->name == NULL ? "<n/a>" : t->name, t->this.e.sort_order);
  if (!detail) return TRUE;
  ioprint(io, "%s       func:%08x  flags:%08x\n", tab,
      t->func, t->flags);
  ioprint(io, "%s       sp:  %08x", tab,
      t->sp);
#if OS_STACK_CHECK
  ioprint(io, " [%s]  ", (t->sp < t->stack_start || t->sp > t->stack_end) ? TEXT_BAD("BPTR") :" ok ");
  bool sp_start_bad = t->stack_start < RAM_BEGIN || t->stack_start > RAM_END;
  bool sp_end_bad = t->stack_end < t->stack_start || t->stack_end > RAM_END;
  ioprint(io, "  sp_start:%08x [%s]  sp_end:%08x [%s]",
      t->stack_start,
      sp_start_bad ? TEXT_BAD("BADR") : (*(u32_t*)(t->stack_start - 4) != OS_STACK_START_MARKER ? TEXT_BAD("CRPT") : " ok "),
      t->stack_end,
      sp_end_bad ? TEXT_BAD("BADR") : (*(u32_t*)(t->stack_end) != OS_STACK_END_MARKER ? TEXT_BAD("CRPT") : " ok ")
          );
#if OS_STACK_USAGE_CHECK
  if (!sp_start_bad && !sp_end_bad) {
    // for descending stacks
    u32_t used_entries = 0;
    u32_t *s = (u32_t *)t->stack_start;
    while (s[used_entries] == _STACK_USAGE_MARK_32 && (void *)&s[used_entries] < t->stack_end) {
      used_entries++;
    }
    u32_t perc = 100 - ((100 * used_entries) / ((t->stack_end - t->stack_start)/sizeof(u32_t)));
    ioprint(io, "  used:%i%", perc);
    if (perc == 100) {
      ioprint(io, TEXT_BAD(" FULL"));
    } else if (perc > 90) {
      ioprint(io, TEXT_NOTE(" ALMOST FULL"));
    }
  }
#endif
#endif
  ioprint(io, "\n");

  if (!list_is_empty(&t->q_join)) {
    ioprint(io, "%s       Join List (%i): \n", tab,
        list_count(&t->q_join));
    OS_DBG_print_thread_list(io, &t->q_join, FALSE, indent + 9);
  }
  return TRUE;
}

static void OS_DBG_print_thread_list(u8_t io, list_t *l, bool detail, int indent) {
  element_t *cur = list_first(l);
  while (cur) {
    os_type type = OS_TYPE(OS_OBJ(cur));
    switch (type) {
    case OS_THREAD:
      OS_DBG_print_thread(io, OS_THREAD(cur), detail, indent);
      break;
    case OS_COND:
      OS_DBG_print_cond(io, OS_COND(cur), detail, indent);
      break;
    default:
      // TODO
      break;
    }
    cur = list_next(cur);
  }
}

static void OS_DBG_list_threads(u8_t io) {
  int i;
  ioprint(io, "Running\n-------\n");
  OS_DBG_print_thread(io, os.current_thread, TRUE, 2);
  ioprint(io, "Scheduled\n---------\n");
  OS_DBG_print_thread_list(io, &os.q_running, TRUE, 2);
  ioprint(io, "Sleeping\n--------\n");
  OS_DBG_print_thread_list(io, &os.q_sleep, TRUE, 2);
  ioprint(io, "Thread peers\n------------\n");
  for (i = 0; i < OS_THREAD_PEERS; i++) {
    OS_DBG_print_thread(io, os.thread_peers[i], TRUE, 2);
  }
}

bool OS_DBG_print_mutex(u8_t io, os_mutex *m, bool detail, int indent) {
  if (m == NULL) return FALSE;
  char tab[32];
  memset(tab, ' ', sizeof(tab));
  tab[indent] = 0;
  ioprint(io, "%smutex  id:%04x  addr:%08x  lock:%08x  attr:%08x  depth:%i\n", tab, m->id, m, m->lock, m->attrs, m->depth);
  if (!detail) return TRUE;
  ioprint(io, "%s       owner: ", tab);
  if (!OS_DBG_print_thread(io, m->owner, FALSE, indent+2)) {
    ioprint(io, "\n");
  }
  ioprint(io, "%s       entries:%i  exits:%i\n", tab, m->entered, m->exited);
  if (!list_is_empty(&m->q_block)) {
    ioprint(io, "%s       Blocked List (%i)\n", tab, list_count(&m->q_block));
    OS_DBG_print_thread_list(io, &m->q_block, FALSE, indent + 9);
  }
  return TRUE;

}

static void OS_DBG_list_mutexes(u8_t io) {
  int i;
  ioprint(io, "Mutex peers\n-----------\n");
  for (i = 0; i < OS_MUTEX_PEERS; i++) {
    OS_DBG_print_mutex(io, os.mutex_peers[i], 2, TRUE);
  }
}

bool OS_DBG_print_cond(u8_t io, os_cond *c, bool detail, int indent) {
  if (c == NULL) return FALSE;
  char tab[32];
  memset(tab, ' ', sizeof(tab));
  tab[indent] = 0;
  ioprint(io, "%scond   id:%04x  addr:%08x  sleepers:%s  order:%08x\n", tab,
      c->id, c,  c->has_sleepers? "YES":"NO ", c->this.e.sort_order);
  ioprint(io, "%s       mutex: ", tab);
  if (!OS_DBG_print_mutex(io, c->mutex, FALSE, indent + 2)) {
    ioprint(io, "\n");
  }
  if (!detail) return TRUE;
  ioprint(io, "%s       waits:%i  signals:%i  broadcasts:%i\n", tab,
      c->waiting, c->signalled, c->broadcasted);
  if (!list_is_empty(&c->q_block)) {
    ioprint(io, "%s       Blocked List (%i)\n", tab, list_count(&c->q_block));
    OS_DBG_print_thread_list(io, &c->q_block, FALSE, indent + 9);
  }
  if (!list_is_empty(&c->q_sleep)) {
    ioprint(io, "%s       TimedWait List (%i)\n", tab, list_count(&c->q_sleep));
    OS_DBG_print_thread_list(io, &c->q_sleep, FALSE, indent + 9);
  }
  return TRUE;
}

static void OS_DBG_list_conds(u8_t io) {
  int i;
  ioprint(io, "Cond peers\n----------\n");
  for (i = 0; i < OS_COND_PEERS; i++) {
    OS_DBG_print_cond(io, os.cond_peers[i], 2, TRUE);
  }
}

void OS_DBG_list_all(u8_t io, bool previous_preempt) {
  ioprint(io, "OS INFO\n-------\n");
  ioprint(io, "  Scheduled threads: %i\n", list_count(&os.q_running));
  ioprint(io, "  Sleeping entries:  %i\n", list_count(&os.q_sleep));
  ioprint(io, "  Spawned threads:   %i\n", g_thr_id);
  ioprint(io, "  Critical depth:    %i\n", g_crit_entry);
  ioprint(io, "  Preemption:        %s\n", previous_preempt ? "ON":"OFF");
  ioprint(io, "  Now:               %i\n", SYS_get_time_ms());
  ioprint(io, "  First awake:       %i (%i in future)\n", os.first_awake, os.first_awake - SYS_get_time_ms());
  OS_DBG_list_threads(io);
  OS_DBG_list_mutexes(io);
  OS_DBG_list_conds(io);
}

void OS_DBG_dump(u8_t io) {
  bool pre = OS_enter_critical();
  OS_DBG_list_all(io, pre);
  OS_exit_critical(pre);
}

#ifdef OS_DUMP_IRQ
void OS_DBG_dump_irq(u8_t io) {
  if(EXTI_GetITStatus(OS_DUMP_IRQ_EXTI_LINE) != RESET) {
    enter_critical();
    OS_DBG_list_all(io, os.preemption);
    exit_critical();
    EXTI_ClearITPendingBit(OS_DUMP_IRQ_EXTI_LINE);
  }
}

#endif

os_thread *OS_DBG_get_thread_by_id(u32_t id) {
#if OS_DBG_MON
  int i;
  for (i = 0; i < OS_THREAD_PEERS; i++) {
    if (os.thread_peers[i] != NULL && os.thread_peers[i]->id == id) {
      return os.thread_peers[i];
    }
  }
  return NULL;
#else
  return NULL;
#endif
}

os_thread **OS_DBG_get_thread_peers() {
#if OS_DBG_MON
  return os.thread_peers;
#else
  return NULL;
#endif
}
#endif // OS_DBG_MON


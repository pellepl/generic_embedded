/*
 * taskq.c
 *
 *  Created on: Jul 18, 2012
 *      Author: petera
 */

#include "taskq.h"
#include "miniutils.h"
#include "system.h"
#include "linker_symaccess.h"
#ifdef CONFIG_OS
#include "os.h"
#endif

static struct {
  volatile task* head;
  volatile task* last;
  task* current;
  task_timer *first_timer;
  volatile bool tim_lock;
#ifdef CONFIG_OS
  os_cond cond;
#endif
} task_sys;

static struct {
  task task[_TASK_POOL];
  u32_t mask[((_TASK_POOL-1)/32)+1];
} task_pool;

static u8_t _g_timer_ix = 0;

static void TASK_insert_timer(task_timer *timer, time actual_time);

static void print_task(u8_t io, task *t, const char *prefix) {
  if (t) {
    ioprint(io, "%s ix:%02x  f:%08x  FLAGS:%s%s%s%s  arg:%08x  argp:%08x\n"
        "%s        next:%08x  run_reqs:%i\n"
      ,
      prefix,
      t->_ix,
      t->f,
      t->flags & TASK_EXE ? " E" : "/e",
      t->flags & TASK_RUN ? " R" : "/r",
      t->flags & TASK_LOOP ? " L" : "/l",
      t->flags & TASK_STATIC ? " S" : "/s",
      t->arg,
      t->arg_p,
      prefix,
      t->_next,
      t->run_requests);
  } else {
    ioprint(io, "%s NONE\n", prefix);
  }
}

static void print_timer(u8_t io, task_timer *t, const char *prefix, time now) {
  if (t) {
    ioprint(io, "%s %s  start:%08x (%+08x)  recurrent:%08x  next:%08x  [%s]\n",
        prefix,
        t->alive ? "ALIVE": "DEAD ",
        (u32_t)t->start_time,
        (u32_t)(t->start_time - now),
        (u32_t)(t->recurrent_time),
        (u32_t)(t->_next),
        t->name);
    print_task(io, t->task, prefix);
  } else {
    ioprint(io, "%s NONE\n", prefix);
  }
}

#define TASK_DUMP_OUTPUT "  task list __ "
#define TASK_TIM_DUMP_OUTPUT "  tmr list __ "
void TASK_dump_pool(u8_t io) {
  int i, j;
  for (i = 0; i <= (_TASK_POOL-1)/32; i++) {
       if (task_pool.mask[i]) {
         for (j = 0; j < 32; j++) {
           int ix = i*32+j;
           ioprint(io, "TASK %i @ %08x\n", ix, &task_pool.task[ix]);
           print_task(io, &task_pool.task[ix], "");
         }
       }
     }
}

void TASK_dump(u8_t io) {
  ioprint(io, "TASK SYSTEM\n-----------\n");
  print_task(io, task_sys.current, "  current");

  char lst[sizeof(TASK_DUMP_OUTPUT)];
  memcpy(lst, TASK_DUMP_OUTPUT, sizeof(TASK_DUMP_OUTPUT));
  char* p = (char*)strchr(lst, '_');
  task* ct = (task *)task_sys.head;
  int ix = 1;
  while (ct) {
    sprint(p, "%02i", ix++);
    print_task(io, ct, lst);
    ct = ct->_next;
  }
  print_task(io, (task *)task_sys.last, "  last   ");

  ioprint(io, "  pool bitmap ");
  for (ix = 0; ix < sizeof(task_pool.mask)/sizeof(task_pool.mask[0]); ix++) {
    ioprint(io, "%032b ", task_pool.mask[ix]);
  }
  ioprint(io, "\n");

  ioprint(io, "  timers\n");
  char lst2[sizeof(TASK_TIM_DUMP_OUTPUT)];
  memcpy(lst2, TASK_TIM_DUMP_OUTPUT, sizeof(TASK_TIM_DUMP_OUTPUT));
  p = (char*)strchr(lst2, '_');
  task_timer* tt = task_sys.first_timer;
  ix = 1;
  time now = SYS_get_time_ms();
  while (tt) {
    sprint(p, "%02i", ix++);
    print_timer(io, tt, lst2, now);
    tt = tt->_next;
  }
}

void TASK_init() {
  int i;
  DBG(D_TASK, D_DEBUG, "TASK init\n");
  memset(&task_sys, 0, sizeof(task_sys));
  memset(&task_pool, 0, sizeof(task_pool));
  for (i = 0; i < _TASK_POOL; i++) {
    task_pool.task[i]._ix = i;
    task_pool.mask[i/32] |= (1<<(i&0x1f));
  }
#ifdef CONFIG_OS
  OS_cond_init(&task_sys.cond);
#endif
}

static task* TASK_snatch_free(int dir) {
  int i, j;
  if (dir) {
    for (i = (_TASK_POOL-1)/32; i >= 0; i--) {
      if (task_pool.mask[i]) {
        for (j = 31; j >= 0; j--) {
          if (task_pool.mask[i] & (1<<j)) {
            goto task_found;
          }
        }
      }
    }
  } else {
    for (i = 0; i <= (_TASK_POOL-1)/32; i++) {
      if (task_pool.mask[i]) {
        for (j = 0; j < 32; j++) {
          if (task_pool.mask[i] & (1<<j)) {
            goto task_found;
          }
        }
      }
    }
  }
  return 0;
task_found:
  task_pool.mask[i] &= ~(1<<j);
  task* task = &task_pool.task[i*32+j];
  return task;

}

task* TASK_create(task_f f, u8_t flags) {
  enter_critical();
  task* task = TASK_snatch_free(flags & TASK_STATIC);
  exit_critical();
  if (task) {
    task->f = f;
    task->run_requests = 0;
    task->flags = flags & (~(TASK_RUN | TASK_EXE));
    return task;
  } else {
    return 0;
  }
}

void TASK_loop(task* task, u32_t arg, void* arg_p) {
  task->flags |= TASK_LOOP;
  TASK_run(task, arg, arg_p);
}

void TASK_run(task* task, u32_t arg, void* arg_p) {
  ASSERT((task->flags & TASK_RUN) == 0);
  ASSERT(task >= &task_pool.task[0]);
  ASSERT(task <= &task_pool.task[_TASK_POOL]);
  task->flags |= TASK_RUN;
  task->arg = arg;
  task->arg_p = arg_p;

  enter_critical();
  if (task_sys.last == 0) {
    task_sys.head = task;
  } else {
    task_sys.last->_next = task;
  }
  task_sys.last = task;
  // would same task be added twice or more, this at least fixes endless loop
  task->_next = 0;
  task->run_requests++;
  TRACE_TASK_RUN(task->_ix);
#if defined(CONFIG_OS) & defined(CONFIG_TASK_QUEUE_IN_THREAD)
  //#if defined(CONFIG_OS)
  // TODO PETER FIX
  // for some reason we sporadically crash when doing OS_cond_sig
  // whilst firing off tasks in IRQs which occur very frequently
  // (eg dumping pics from ADNS3000s) - only when task queue is not in
  // a thread. Fix is to put task queue in a thread or not signalling
  // the condition, but need to sort out why this happens.
  //
  // hardfault:
  //  INVPC: UsaFlt general
  //  FORCED: HardFlt SVC/BKPT within SVC

  OS_cond_signal(&task_sys.cond);
#endif
  exit_critical();
}

void TASK_start_timer(task *task, task_timer* timer, u32_t arg, void *arg_p, time start_time, time recurrent_time,
    const char *name) {
  enter_critical();
  task_sys.tim_lock = TRUE;
  ASSERT(timer->alive == FALSE);
  timer->_ix = _g_timer_ix++;
  timer->arg = arg;
  timer->arg_p = arg_p;
  timer->task = task;
  timer->start_time = start_time;
  timer->recurrent_time = recurrent_time;
  timer->alive = TRUE;
  timer->name = name;
  TASK_insert_timer(timer, SYS_get_time_ms() + start_time);
  task_sys.tim_lock = FALSE;
  exit_critical();
}

void TASK_set_timer_recurrence(task_timer* timer, time recurrent_time) {
  timer->recurrent_time = recurrent_time;
}

void TASK_stop_timer(task_timer* timer) {
  enter_critical();
  task_sys.tim_lock = TRUE;
  timer->alive = FALSE;

  // wipe all dead instances
  task_timer *cur_timer = task_sys.first_timer;
  task_timer *pre_timer = NULL;
  while (cur_timer) {
    if (cur_timer->alive == FALSE) {
      if (pre_timer == NULL) {
        task_sys.first_timer = timer->_next;
      } else {
        pre_timer->_next = timer->_next;
      }
    }
    pre_timer = cur_timer;
    cur_timer = cur_timer->_next;
  }
  task_sys.tim_lock = FALSE;
  exit_critical();
}

void TASK_kill() {
  task_sys.current->flags &= ~(TASK_LOOP | TASK_RUN);
}

u32_t TASK_id() {
  return task_sys.current->_ix;
}

u8_t TASK_is_running(task* t) {
  return t->flags & TASK_RUN;
}

void TASK_wait() {
  while (task_sys.head == 0) {
#if defined(CONFIG_OS) & defined(CONFIG_TASK_QUEUE_IN_THREAD)
    OS_cond_wait(&task_sys.cond, NULL);
#else
    arch_sleep();
#endif
  }
}

void TASK_free(task *t) {
  enter_critical();
  task_pool.mask[t->_ix/32] |= (1<<(t->_ix & 0x1f));
  t->flags &= ~(TASK_RUN | TASK_STATIC);
  exit_critical();
}

u32_t TASK_tick() {
  enter_critical();
  volatile task* t = task_sys.head;
  task_sys.current = (task *)t;
  if (t == 0) {
    // naught to do
    exit_critical();
    return 0;
  }
  ASSERT(t >= &task_pool.task[0]);
  ASSERT(t <= &task_pool.task[_TASK_POOL]);

  // execute
  char do_run = t->flags & TASK_RUN;

  // first, fiddle with queue
  // reinsert or kill off
  if ((t->flags & TASK_LOOP)) {
    // loop, put this at end or simply keep if the only one
    if (t->_next != 0) {
      task_sys.last->_next = (task *)t;
      task_sys.head = t->_next;
      if (task_sys.head != 0) {
        ASSERT(task_sys.head >= &task_pool.task[0]);
        ASSERT(task_sys.head <= &task_pool.task[_TASK_POOL]);
      }
      task_sys.last = t;
    }
  } else {
    // no loop, kill off
    task_sys.head = t->_next;
    if (t->_next == 0) {
      task_sys.last = 0;
    } else {
      ASSERT(task_sys.head >= &task_pool.task[0]);
      ASSERT(task_sys.head <= &task_pool.task[_TASK_POOL]);
    }
    // free unless static
    if ((t->flags & TASK_STATIC) == 0) {
      task_pool.mask[t->_ix/32] |= (1<<(t->_ix & 0x1f));
    }
    t->flags &= ~TASK_RUN;
  }
  exit_critical();

  if (do_run) {
    u8_t run_requests;
    // execute
#if TASK_WARN_HIGH_EXE_TIME > 0
    time then = SYS_get_tick();
#endif
    t->flags |= TASK_EXE;
    TRACE_TASK_ENTER(t->_ix);
    do {
      t->f(t->arg, t->arg_p);

      enter_critical();
      if (t->run_requests > 0) {
        t->run_requests--;
      }
      run_requests = t->run_requests;
      exit_critical();
    } while (run_requests > 0);
    TRACE_TASK_EXIT(t->_ix);
    t->flags &= ~TASK_EXE;
#if TASK_WARN_HIGH_EXE_TIME > 0
    time delta = SYS_get_tick() - then;
    if (delta >= TASK_WARN_HIGH_EXE_TIME) {
      DBG(D_TASK, D_WARN, "TASK task %p: %i ticks\n", t->f, delta);
    }
#endif
  }

  return 1;
}

static void TASK_insert_timer(task_timer *timer, time actual_time) {
  timer->start_time = actual_time;

  if (task_sys.first_timer == 0) {
    // first only timer entry
    timer->_next = 0;
    task_sys.first_timer = timer;
  } else {
    task_timer *cur_timer = task_sys.first_timer;
    task_timer *prev_timer = 0;
    int guard = 0;
    while (cur_timer && timer->start_time >= cur_timer->start_time) {
      prev_timer = cur_timer;
      cur_timer = cur_timer->_next;
      guard++;
      ASSERT(guard < 128);
    }
    if (prev_timer == 0) {
      // earliest timer entry
      timer->_next = task_sys.first_timer;
      task_sys.first_timer = timer;
    } else if (cur_timer == 0) {
      // latest timer entry
      prev_timer->_next = timer;
      timer->_next = 0;
    } else {
      // in the middle
      prev_timer->_next = timer;
      timer->_next = cur_timer;
    }
  }
}

void TASK_timer() {
  task_timer *cur_timer = task_sys.first_timer;
  if (cur_timer == NULL || task_sys.tim_lock) {
    return;
  }

  task_timer *old_timer = NULL;
  while (cur_timer && cur_timer->start_time <= SYS_get_time_ms()) {
    enter_critical();
    if (!TASK_is_running(cur_timer->task) && cur_timer->alive) {
      // expired, schedule for run
      TRACE_TASK_TIMER(cur_timer->_ix);
      TASK_run(cur_timer->task, cur_timer->arg, cur_timer->arg_p);
    }
    old_timer = cur_timer;
    cur_timer = cur_timer->_next;
    task_sys.first_timer = cur_timer;
    if (old_timer->recurrent_time && old_timer->alive) {
      // recurrent, reinsert
      old_timer->start_time += old_timer->recurrent_time; // need to set this before inserting for sorting
      TASK_insert_timer(old_timer, old_timer->start_time);
    } else {
      old_timer->alive = FALSE;
    }
    exit_critical();
  }
}

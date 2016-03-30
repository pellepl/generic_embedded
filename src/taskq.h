/*
 * taskq.h
 *
 *  Created on: Jul 18, 2012
 *      Author: petera
 */

#ifndef TASKQ_H_
#define TASKQ_H_

#include "system.h"
#include "types.h"

#ifndef TASK_WARN_HIGH_EXE_TIME
#define TASK_WARN_HIGH_EXE_TIME     0
#endif

#ifndef CONFIG_TASK_POOL
#define CONFIG_TASK_POOL      64
#endif

/* Flag for a task that is scheduled to run in next TASK_tick */
#define TASK_RUN        (1<<0)
/* Flag for a task that will be rescheduled after each execution  */
#define TASK_LOOP       (1<<1)
/* Flag for a task that will remain allocated after execution */
#define TASK_STATIC     (1<<2)
/* Flag for a task that is killed */
#define TASK_KILLED     (1<<3)
/* Flag for a task that waits for a mutex to unlock */
#define TASK_WAIT       (1<<6)
/* Flag for an executing task */
#define TASK_EXE        (1<<7)

typedef void(*task_f)(u32_t arg, void* arg_p);

#ifdef CONFIG_TASKQ_MUTEX
struct task_mutex_s;
#endif

typedef struct task_s {
  u8_t _ix;
  u8_t flags;
  u8_t run_requests;
  u32_t arg;
  void* arg_p;
  task_f f;
#ifdef CONFIG_TASKQ_MUTEX
  struct task_mutex_s *wait_mutex;
#endif
  struct task_s *_next;
} task;

typedef struct task_timer_s {
  u8_t _ix;
  task *task;
  sys_time start_time;
  sys_time recurrent_time;
  u32_t arg;
  void* arg_p;
  bool alive;
  const char *name;
  struct task_timer_s *_next;
} task_timer;

#ifdef CONFIG_TASKQ_MUTEX
typedef struct task_mutex_s {
  volatile bool taken;
  bool reentrant;
  u8_t entries;
  task *owner;
  volatile task* head;
  volatile task* last;
} task_mutex;
#define TASK_MUTEX_INIT {FALSE,FALSE,0,NULL,NULL,NULL}
#define TASK_MUTEX_REENTRANT_INIT {FALSE,TRUE,0,NULL,NULL,NULL}
#endif

/**
 * Initializes the task system
 */
void TASK_init();
/**
 * Creates a new task
 * @param f the task function of this task
 * @param flags TASK_STATIC for static allocation of task (memory never released) or 0 for dynamic
 * @returns the task or NULL if no free psace
 */
task* TASK_create(task_f f, u8_t flags);
/**
 * Loops this task until TASK_kill is called upon it
 * @param task the task to loop
 * @param arg the value argument to task function
 * @param arg_p the pointer argument to task function
 */
void TASK_loop(task* task, u32_t arg, void* arg_p);
/**
 * Runs this task
 * @param task the task to run
 * @param arg the value argument to task function
 * @param arg_p the pointer argument to task function
 */
void TASK_run(task* task, u32_t arg, void* arg_p);
/**
 * Prevents currently running task to be executed again. Mainly
 * used for looping tasks. If this task has just been entered
 * into a mutex's wait queue, it will not be executed once the mutex
 * is released.
 */
void TASK_stop();
/**
 * Returns id of currently running task
 */
u32_t TASK_id();
/**
 * Returns whether given task is running or not
 */
u8_t TASK_is_running(task* t);
/**
 * Starts a timer which will execute given task within given time with given args.
 * May be recurrent.
 * @param task the task to schedule
 * @param timer the timer for this task
 * @param arg the value argument to task function
 * @param arg_p the pointer argument to task function
 * @param start_time the delta time from now to schedule task
 * @param recurrent_time the delta time of recurrence, or 0 for no recurrence
 */
void TASK_start_timer(task *task, task_timer* timer, u32_t arg, void *arg_p, sys_time start_time, sys_time recurrent_time,
    const char *name);
/**
 * Reschedules given timer for new recurrent time.
 * New reschedule time won't be effective until timer has been executed after
 * this invocation
 */
void TASK_set_timer_recurrence(task_timer* timer, sys_time recurrent_time);
/**
 * Kills of given timer
 */
void TASK_stop_timer(task_timer* timer);

/**
 * Frees a static task, or a task that has been inadvertedly constructed and must
 * never run. The task will be removed from any mutex's wait queue.
 */
void TASK_free(task *t);

#ifdef CONFIG_TASKQ_MUTEX
/**
 * Tries to lock given mutex. If function returns FALSE, the task must behave well and return directly,
 * or at least before using any resources protected by the mutex.
 * Returns TRUE if the mutex was taken, or FALSE if the mutex was busy and we're put in waiting state.
 */
bool TASK_mutex_lock(task_mutex *m);
/**
 * Tries to lock given mutex. If function returns FALSE, the task must not use any resources protected
 * by the mutex.
 * Returns TRUE if the mutex was taken, or FALSE if the mutex was busy.
 */
bool TASK_mutex_try_lock(task_mutex *m);
/**
 * Unlocks given mutex and reschedules all tasks that are waiting for the mutex.
 */
void TASK_mutex_unlock(task_mutex *m);
#endif

/**
 * Executes one pending task and returns 1. If there is no pending task, this function
 * returns 0.
 * Common way to call this function:
 * while (TRUE) {
 *   while (TASK_tick());
 *   // target sleep or something, e.g. TASK_wait()
 * }
 */
u32_t TASK_tick();
/**
 * Depending on build time config, will either suspend thread that is execution tasks
 * or will call arch_sleep.
 */
void TASK_wait();
/**
 * Checks timers and schedules tasks for execution whose wakeup have elapsed.
 */
void TASK_timer();
/**
 * Returns ms for nearest awaiting timer
 */
s32_t TASK_next_wakeup_ms(sys_time *t, task_timer **timer);
/**
 * Returns if there are any active tasks
 */
bool TASK_got_active_tasks(void);

void TASK_dump(u8_t io);
void TASK_dump_pool(u8_t io);

#endif /* TASKQ_H_ */

/*
 * taskq.h
 *
 *  Created on: Jul 18, 2012
 *      Author: petera
 */

#ifndef TASKQ_H_
#define TASKQ_H_

#include "types.h"

#define TASK_WARN_HIGH_EXE_TIME     40

#define _TASK_POOL      64

#define TASK_RUN        (1<<0)
#define TASK_LOOP       (1<<1)
#define TASK_STATIC     (1<<2)
#define TASK_EXE        (1<<7)

typedef void(*task_f)(u32_t arg, void* arg_p);

typedef struct task_s {
  u8_t _ix;
  u8_t flags;
  u8_t run_requests;
  u32_t arg;
  void* arg_p;
  task_f f;
  struct task_s *_next;
} task;

typedef struct task_timer_s {
  u8_t _ix;
  task *task;
  time start_time;
  time recurrent_time;
  u32_t arg;
  void* arg_p;
  bool alive;
  const char *name;
  struct task_timer_s *_next;
} task_timer;

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
 * Kills currently running task
 */
void TASK_kill();
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
void TASK_start_timer(task *task, task_timer* timer, u32_t arg, void *arg_p, time start_time, time recurrent_time,
    const char *name);
/**
 * Reschedules given timer for new recurrent time.
 * New reschedule time won't be effective until timer has been executed after
 * this invocation
 */
void TASK_set_timer_recurrence(task_timer* timer, time recurrent_time);
/**
 * Kills of given timer
 */
void TASK_stop_timer(task_timer* timer);

/**
 * Frees a static task, or a task that has been inadvertedly constructed and must
 * never run.
 */
void TASK_free(task *t);

u32_t TASK_tick();
void TASK_wait();
void TASK_timer();
void TASK_dump();

#endif /* TASKQ_H_ */

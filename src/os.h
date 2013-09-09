/*
 * os.h
 *
 *  Created on: Mar 17, 2013
 *      Author: petera
 */

#ifndef OS_H_
#define OS_H_

#include "system.h"
#include "list.h"

// do not modify without altering asm code in os.c
#define OS_THREAD_FLAG_PRIVILEGED   (1<<2)

typedef enum os_type_e {
  OS_THREAD = 0,
  OS_COND
} os_type;

typedef struct os_object_t {
  element_t e;
  os_type type;
} os_object;

typedef struct os_thread_t {
  os_object this;
  u32_t id;
  u8_t prio;
  void * sp; // The current stack pointer
  u32_t flags; // Status flags
  list_t q_join;
  void * stack_start;
  void * stack_end;
  void *(*func)(void *);
  const char *name;
  u32_t ret_val;
} os_thread;

typedef struct os_mutex_t {
  u32_t id;
  u32_t lock;
  u32_t attrs;
  os_thread *owner;
  list_t q_block;
  u16_t depth;
#if OS_DBG_MON
  u32_t entered;
  u32_t exited;
#endif
} os_mutex;

typedef struct os_cond_t {
  os_object this;
  u32_t id;
  os_mutex *mutex;
  list_t q_block;
  list_t q_sleep;
  bool has_sleepers;
#if OS_DBG_MON
  u32_t waiting;
  u32_t signalled;
  u32_t broadcasted;
#endif
} os_cond;

/**
 * Enabling this flag will make thread enter critical section
 * when mutex is taken and release the critical section when
 * mutex is released
 */
#define OS_MUTEX_ATTR_CRITICAL_IRQ     (1<<0)
/**
 * Enabling this flag will make thread reset critical section
 * to full exit when mutex is released
 */
#define OS_MUTEX_ATTR_CRITICAL_EXIT    (1<<1)
#define OS_MUTEX_ATTR_REENTRANT        (1<<2)

u32_t OS_thread_create(os_thread *t, u32_t flags, void *(*func)(void *), void *arg, void *stack, u32_t stack_size, const char *name);
u32_t OS_thread_id(os_thread *t);
os_thread *OS_thread_self();
u32_t OS_thread_self_id();
u32_t OS_thread_yield(void);
void OS_thread_join(os_thread *t);

void OS_thread_sleep(time t);

u32_t OS_mutex_init(os_mutex *m, u32_t attrs);
u32_t OS_mutex_lock(os_mutex *m);
u32_t OS_mutex_unlock(os_mutex *m);
bool OS_mutex_try_lock(os_mutex *m);

u32_t OS_cond_init(os_cond *c);
u32_t OS_cond_wait(os_cond *c, os_mutex *m);
u32_t OS_cond_timed_wait(os_cond *c, os_mutex *m, time t);
u32_t OS_cond_signal(os_cond *c);
u32_t OS_cond_broadcast(os_cond *c);

void OS_svc_0(void *arg, ...);
void OS_svc_1(void *arg, ...);
void OS_svc_2(void *arg, ...);
void OS_svc_3(void *arg, ...);

void OS_init(void);

void __os_enter_critical_kernel(void);
void __os_exit_critical_kernel(void);

void __os_systick(void);
void __os_pendsv(void);
void __os_time_tick(time now);

os_thread *OS_DBG_get_thread_by_id(u32_t id);

#if OS_DBG_MON
bool OS_DBG_print_thread(os_thread *t, bool detail, int indent);
bool OS_DBG_print_cond(os_cond *c, bool detail, int indent);
bool OS_DBG_print_mutex(os_mutex *m, bool detail, int indent);
void OS_DBG_dump();
void OS_DBG_list_all(bool prev_preempt);
#ifdef OS_DUMP_IRQ
void OS_DBG_dump_irq();
#endif
#endif

#endif /* OS_H_ */

/*
 * shared_mem.h
 *
 *  Created on: Apr 6, 2013
 *      Author: petera
 */

#ifndef SHARED_MEM_H_
#define SHARED_MEM_H_

#include "system.h"

enum reboot_reason_e {
  REBOOT_COLD_START = 0,
  REBOOT_UNKONWN,
  REBOOT_USER,
  REBOOT_ASSERT,
  REBOOT_CRASH,
  REBOOT_EXEC_BOOTLOADER,
  REBOOT_BOOTLOADER,
};

typedef struct {
  u32_t magic;
  enum reboot_reason_e reboot_reason;
  u32_t user[16];
  u32_t chk;
} shmem;

#define SHMEM_CALC_CHK(shmem_p, dst) do { \
  u32_t *p = (u32_t *)(shmem_p); \
  u32_t chk = 0; \
  int i; \
  for (i = 0; i < sizeof(shmem)/4-1; i++) { \
    chk ^= p[i]; \
  } \
  (dst) = chk; \
} while(0);

bool SHMEM_validate();
shmem *SHMEM_get();
void SHMEM_set_reboot_reason(enum reboot_reason_e rr);


#endif /* SHARED_MEM_H_ */

#include "system.h"
#include "shared_mem.h"
#include "linker_symaccess.h"

#define SHMEM_MAGIC 0xd0decaed

#define __shmem ((shmem *)(SHARED_MEMORY_ADDRESS))


static void _shmem_reset() {
  memset(__shmem, 0, sizeof(shmem));
  SHMEM_get()->magic = SHMEM_MAGIC;
  SHMEM_get()->reboot_reason = REBOOT_COLD_START;
  SHMEM_CALC_CHK(__shmem, SHMEM_get()->chk);
}

bool SHMEM_validate() {
  bool ok = FALSE;
  do {
    if (SHMEM_get()->magic != SHMEM_MAGIC) {
      break;
    }
    u32_t exp_chk;
    SHMEM_CALC_CHK(__shmem, exp_chk);

    if (SHMEM_get()->chk != exp_chk) {
      break;
    }
    ok = TRUE;
  } while (0);
  if (!ok) {
    _shmem_reset();
  }
  return ok;
}

shmem *SHMEM_get() {
  return __shmem;
}

void SHMEM_set_reboot_reason(enum reboot_reason_e rr) {
  SHMEM_get()->reboot_reason = rr;
  SHMEM_CALC_CHK(__shmem, SHMEM_get()->chk);
}

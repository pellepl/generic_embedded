#include "spi_flash_os.h"
#include "spi_flash.h"
#include "taskq.h"
#include "os.h"
#include "spi_flash_m25p16.h"
#include "miniutils.h"

enum sfos_op {
  FS_OP_IDLE = 0,
  SFOS_OP_ERASE,
  SFOS_OP_READ,
  SFOS_OP_WRITE,
};

struct {
  os_mutex lock;
  os_mutex sig_mutex;
  os_cond cond;
  s32_t sig_res;
  enum sfos_op state;
  task *kernel_task;
  u32_t args[3];
} sfos;


static void sfos_signal_result(s32_t res) {
  OS_mutex_lock(&sfos.sig_mutex);
  sfos.sig_res = res;
  sfos.state = FS_OP_IDLE;
  OS_cond_broadcast(&sfos.cond);
  OS_mutex_unlock(&sfos.sig_mutex);
}

static void sfos_spi_flash_cb(spi_flash_dev *dev, int result) {
  sfos_signal_result(result);
}

static s32_t sfos_exe(enum sfos_op op) {
  s32_t res;
  OS_mutex_lock(&sfos.sig_mutex);
  sfos.state = op;
  TASK_run(sfos.kernel_task, op, sfos.args);
  while (sfos.state != FS_OP_IDLE) {
    OS_cond_wait(&sfos.cond, &sfos.sig_mutex);
  }
  res = sfos.sig_res;
  OS_mutex_unlock(&sfos.sig_mutex);
  return res;
}

static void sfos_task_f(u32_t state, void* arg_v) {
  s32_t res;
  u32_t *args = (u32_t*)arg_v;
  switch (sfos.state) {
  case SFOS_OP_ERASE:
    DBG(D_APP, D_DEBUG, "SFOS erase %08x:%08x\n", args[0], args[1]);
    res = SPI_FLASH_erase(SPI_FLASH, sfos_spi_flash_cb, args[0], args[1]);
    break;
  case SFOS_OP_READ:
    DBG(D_APP, D_DEBUG, "SFOS read %08x:%08x\n", args[0], args[1]);
    res = SPI_FLASH_read(SPI_FLASH, sfos_spi_flash_cb, args[0], args[1], (u8_t*)args[2]);
    break;
  case SFOS_OP_WRITE:
    DBG(D_APP, D_DEBUG, "SFOS write %08x:%08x\n", args[0], args[1]);
    res = SPI_FLASH_write(SPI_FLASH, sfos_spi_flash_cb, args[0], args[1], (u8_t*)args[2]);
    break;
  default:
    res = SPI_FLASH_ERR_UNDEFINED_STATE;
    ASSERT(FALSE);
    break;
  }
  if (res != SPI_OK) {
    sfos_signal_result(res);
  }
}

s32_t SFOS_erase(u32_t addr, u32_t size) {
  s32_t res;
  OS_mutex_lock(&sfos.lock);
  sfos.args[0] = addr;
  sfos.args[1] = size;
  res = sfos_exe(SFOS_OP_ERASE);
  OS_mutex_unlock(&sfos.lock);
  return res;
}

s32_t SFOS_read(u32_t addr, u32_t size, u8_t *dst) {
  s32_t res;
  OS_mutex_lock(&sfos.lock);
  sfos.args[0] = addr;
  sfos.args[1] = size;
  sfos.args[2] = (u32_t)dst;
  res = sfos_exe(SFOS_OP_READ);
  OS_mutex_unlock(&sfos.lock);
  return res;
}

s32_t SFOS_write(u32_t addr, u32_t size, u8_t *src) {
  s32_t res;
  OS_mutex_lock(&sfos.lock);
  sfos.args[0] = addr;
  sfos.args[1] = size;
  sfos.args[2] = (u32_t)src;
  res = sfos_exe(SFOS_OP_WRITE);
  OS_mutex_unlock(&sfos.lock);
  return res;
}

void SFOS_init() {
  memset(&sfos, 0, sizeof(sfos));
  OS_mutex_init(&sfos.lock, 0);
  OS_mutex_init(&sfos.sig_mutex, 0);
  OS_cond_init(&sfos.cond);
  sfos.kernel_task = TASK_create(sfos_task_f, TASK_STATIC);
}

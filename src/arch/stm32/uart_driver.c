#include "uart_driver.h"
#include "system.h"

#define UART_HW(u)      ((USART_TypeDef *)((u)->hw))

#if UART_USE_STM_LIB
#define UART_CHECK_RX(u) USART_GetITStatus((u), USART_IT_RXNE) == SET
#define UART_CHECK_TX(u) USART_GetITStatus((u), USART_IT_TXE) == SET
#define UART_CHECK_OR(u) USART_GetITStatus((u), USART_IT_ORE) == SET
#define UART_RX_IRQ_OFF(u) \
    do { \
      USART_ITConfig(UART_HW(u), USART_IT_RXNE, DISABLE); \
    } while (0);
#define UART_RX_IRQ_ON(u) \
    do { \
      USART_ITConfig(UART_HW(u), USART_IT_RXNE, ENABLE); \
    } while (0);
#define UART_TX_IRQ_OFF(u) \
    do { \
      USART_ITConfigUART_HW(u), USART_IT_TXE, DISABLE); \
    } while (0);
#define UART_TX_IRQ_ON(u) \
    do { \
      USART_ITConfig(UART_HW(u), USART_IT_TXE, ENABLE); \
    } while (0);
#else
#define UART_CHECK_RX(u) (UART_HW(u)->SR & USART_SR_RXNE)
#define UART_CHECK_TX(u) (UART_HW(u)->SR & USART_SR_TXE)
#define UART_CHECK_OR(u) (UART_HW(u)->SR & USART_SR_ORE)
#define UART_RX_IRQ_OFF(u) UART_HW(u)->CR1 &= ~USART_CR1_RXNEIE
#define UART_RX_IRQ_ON(u)  UART_HW(u)->CR1 |= USART_CR1_RXNEIE
#define UART_TX_IRQ_OFF(u) UART_HW(u)->CR1 &= ~USART_CR1_TXEIE
#define UART_TX_IRQ_ON(u)  UART_HW(u)->CR1 |= USART_CR1_TXEIE
#endif

#if UART_RECORD_IRQ_TYPE
static void UART_record_irq(uart *u);
#endif

void UART_irq(uart *u) {
  if (u->hw == 0) return;
#if UART_RECORD_IRQ_TYPE
  UART_record_irq(u);
#endif

  if ((UART_CHECK_RX(u)) && (UART_HW(u)->CR1 & USART_CR1_RXNEIE)) {
    u8_t c = USART_ReceiveData(UART_HW(u));
    u->rx.buf[u->rx.wix++] = c;
    if (u->rx.wix >= UART_RX_BUFFER) {
      u->rx.wix = 0;
    }
    if (u->rx_f) {
      u->rx_f(u->arg, c);
    }
  }
  if ((UART_CHECK_TX(u))) {
    if (UART_ALWAYS_SYNC_TX || u->sync_tx) {
      USART_ITConfig(UART_HW(u), USART_IT_TXE, DISABLE);
    } else {
      if (u->tx.wix != u->tx.rix) {
        USART_SendData(UART_HW(u), u->tx.buf[u->tx.rix++]);
        if (u->tx.rix >= UART_TX_BUFFER) {
          u->tx.rix = 0;
        }
      }
      if (u->tx.wix == u->tx.rix) {
        UART_TX_IRQ_OFF(u);
      }
    }
  }
  if (UART_CHECK_OR(u)) {
    (void)USART_ReceiveData(UART_HW(u));
  }
}

u16_t UART_rx_available(uart *u) {
  volatile u16_t r = u->rx.rix;
  volatile u16_t w = u->rx.wix;
  if (w >= r) {
    return w - r;
  } else {
    return w + UART_RX_BUFFER - r;
  }
}

u16_t UART_tx_available(uart *u) {
  volatile u16_t r = u->tx.rix;
  volatile u16_t w = u->tx.wix;
  if (w > r) {
    return UART_TX_BUFFER - (w - r) - 1;
  } else {
    return UART_TX_BUFFER - (w + UART_TX_BUFFER - r) - 1;
  }
}

void UART_tx_drain(uart *uart) {
#if !UART_SYNC_TX
  uart->tx.rix = 0;
  uart->tx.wix = 0;
#endif
}

void UART_tx_flush(uart *uart) {
  while (uart->tx.rix != uart->tx.wix) {
    u8_t c = uart->tx.buf[uart->tx.rix];
    UART_tx_force_char(uart, c);
    if (uart->tx.rix >= UART_RX_BUFFER - 1) {
      uart->tx.rix = 0;
    } else {
      uart->tx.rix++;
    }
  }
}

s32_t UART_get_char(uart *u) {
  s32_t c = -1;
  UART_RX_IRQ_OFF(u);
  if (u->rx.rix != u->rx.wix) {
    c = u->rx.buf[u->rx.rix];
    if (u->rx.rix >= UART_RX_BUFFER - 1) {
      u->rx.rix = 0;
    } else {
      u->rx.rix++;
    }
  }
  UART_RX_IRQ_ON(u);
  return c;
}

s32_t UART_get_buf(uart *u, u8_t* dst, u16_t len) {

  UART_RX_IRQ_OFF(u);
  u16_t avail = UART_rx_available(u);
  s32_t len_to_read = MIN(avail, len);
  if (len_to_read == 0) {
    UART_RX_IRQ_ON(u);
    return 0;
  }
  u32_t remaining = len_to_read;
  u32_t len_to_end = UART_RX_BUFFER - u->rx.rix;
  if (remaining > len_to_end) {
    if (dst) {
      memcpy(dst, &u->rx.buf[u->rx.rix], len_to_end);
      dst += len_to_end;
    }
    remaining -= len_to_end;
    u->rx.rix = 0;
  }
  if (dst) {
    memcpy(dst, &u->rx.buf[u->rx.rix], remaining);
  }
  u->rx.rix += remaining;
  if (u->rx.rix >= UART_RX_BUFFER) {
    u->rx.rix = 0;
  }

  UART_RX_IRQ_ON(u);
  return len_to_read;
}

s32_t UART_put_char(uart *u, u8_t c) {
  if (UART_ALWAYS_SYNC_TX || u->sync_tx) {
    UART_tx_force_char(u, c);
    return 0;
  } else {
    s32_t res;
    int max_tries = 0x10000;
    do {
      res = 0;

      if (UART_tx_available(u) > 0) {
        u->tx.buf[u->tx.wix++] = c;
        if (u->tx.wix >= UART_TX_BUFFER) {
          u->tx.wix = 0;
        }
      } else {
        res = -1;
      }
      UART_TX_IRQ_ON(u);
    } while (u->assure_tx && res != 0 && --max_tries > 0);
    return res;
  }
}

void UART_tx_force_char(uart *u, u8_t c) {
  while (UART_CHECK_TX(u) == 0)
    ;
  USART_SendData(UART_HW(u), (uint8_t) c);
}

s32_t UART_put_buf(uart *u, u8_t* c, u16_t len) {
  if (UART_ALWAYS_SYNC_TX || u->sync_tx) {
    u16_t tlen = len;
    while (tlen-- > 0) {
      UART_put_char(u, *c++);
    }
    return len;
  } else {
    u32_t written = 0;
    u16_t guard = 0;
    do {
      u16_t avail = UART_tx_available(u);
      s32_t len_to_write = MIN(len - written, avail);
      guard++;
      if (len_to_write == 0) {
        while (UART_CHECK_TX(u) == 0)
          ;
        len_to_write = 1;
      }
      guard = 0;
      u32_t remaining = len_to_write;
      u32_t len_to_end = UART_TX_BUFFER - u->tx.wix;
      if (remaining > len_to_end) {
        memcpy(&u->tx.buf[u->tx.wix], c, len_to_end);
        c += len_to_end;
        remaining -= len_to_end;
        u->tx.wix = 0;
      }
      memcpy(&u->tx.buf[u->tx.wix], c, remaining);
      c += remaining;
      u->tx.wix += remaining;
      if (u->tx.wix >= UART_RX_BUFFER) {
        u->tx.wix = 0;
      }
      UART_TX_IRQ_ON(u);
      written += len_to_write;
    } while (u->assure_tx && written < len && guard < 0xff);

    return written;
  }
}

void UART_set_callback(uart *u, uart_rx_callback rx_f, void* arg) {
  u->rx_f = rx_f;
  u->arg = arg;
}

void UART_get_callback(uart *u, uart_rx_callback *rx_f, void **arg) {
  *rx_f = u->rx_f;
  *arg = u->arg;
}

uart __uart_vec[CONFIG_UART_CNT];

void UART_init() {
  memset(__uart_vec, 0, sizeof(__uart_vec));
  int uc = 0;

#ifdef CONFIG_UART1
  _UART(uc++)->hw = USART1;
#endif
#ifdef CONFIG_UART2
  _UART(uc++)->hw = USART2;
#endif
#ifdef CONFIG_UART3
  _UART(uc++)->hw = USART3;
#endif
#ifdef CONFIG_UART4
  _UART(uc++)->hw = UART4;
#endif

  uc = 0;
#ifdef CONFIG_UART1
  UART_TX_IRQ_OFF(_UART(uc++));
#endif
#ifdef CONFIG_UART2
  UART_TX_IRQ_OFF(_UART(uc++));
#endif
#ifdef CONFIG_UART3
  UART_TX_IRQ_OFF(_UART(uc++));
#endif
#ifdef CONFIG_UART4
  UART_TX_IRQ_OFF(_UART(uc++));
#endif
}


void UART_assure_tx(uart *u, bool on) {
  u->assure_tx = on;
}

bool UART_sync_tx(uart *u, bool on) {
  bool old = u->sync_tx;
  u->sync_tx = on;
  return old;
}


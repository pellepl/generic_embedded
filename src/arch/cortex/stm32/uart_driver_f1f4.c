#include "uart_driver.h"
#include "system.h"

#define UART_HW(u)      ((USART_TypeDef *)((u)->hw))

#define UART_CHECK_RX(u) (UART_HW(u)->SR & USART_SR_RXNE)
#define UART_CHECK_TX(u) (UART_HW(u)->SR & USART_SR_TXE)
#define UART_CHECK_OR(u) (UART_HW(u)->SR & USART_SR_ORE)
#define UART_RX_IRQ_OFF(u) UART_HW(u)->CR1 &= ~USART_CR1_RXNEIE
#define UART_RX_IRQ_ON(u)  UART_HW(u)->CR1 |= USART_CR1_RXNEIE
#define UART_TX_IRQ_OFF(u) UART_HW(u)->CR1 &= ~USART_CR1_TXEIE
#define UART_TX_IRQ_ON(u)  UART_HW(u)->CR1 |= USART_CR1_TXEIE

void UART_irq(uart *u) {
  if (u->hw == 0) return;

  if ((UART_CHECK_RX(u)) && (UART_HW(u)->CR1 & USART_CR1_RXNEIE)) {
    u8_t c = UART_HW(u)->DR;
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
      UART_TX_IRQ_OFF(u);
    } else {
      if (u->tx.wix != u->tx.rix) {
        UART_HW(u)->DR = u->tx.buf[u->tx.rix++];
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
    (void)UART_HW(u)->DR;
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

void UART_tx_flush(uart *u) {
  UART_RX_IRQ_OFF(u);
  u16_t rix = u->tx.rix;
  u16_t wix = u->tx.wix;
  while (rix != wix) {
    u8_t c = u->tx.buf[rix];
    UART_tx_force_char(u, c);
    if (rix >= UART_RX_BUFFER - 1) {
      rix = 0;
    } else {
      rix++;
    }
  }
  u->tx.rix = rix;
  UART_RX_IRQ_ON(u);
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
  UART_HW(u)->DR = (uint8_t) c;
}

s32_t UART_put_buf(uart *u, u8_t* c, u16_t len) {
  if (UART_ALWAYS_SYNC_TX || u->sync_tx) {
    u16_t tlen = len;
    while (tlen-- > 0) {
      UART_HW(u)->DR = *c++;
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

bool UART_assure_tx(uart *u, bool on) {
  bool old = u->assure_tx;
  u->assure_tx = on;
  return old;
}

bool UART_sync_tx(uart *u, bool on) {
  bool old = u->sync_tx;
  u->sync_tx = on;
  return old;
}

#ifndef CONFIG_UART_OWN_CFG
bool UART_config(uart *uart, u32_t baud, UART_databits databits,
    UART_stopbits stopbits, UART_parity parity, UART_flowcontrol flowcontrol,
    bool activate) {
  USART_InitTypeDef cfg;

  USART_Cmd(UART_HW(uart), DISABLE);

  if (activate) {
    cfg.USART_BaudRate = baud;
    switch (databits) {
    case UART_CFG_DATABITS_8: cfg.USART_WordLength = USART_WordLength_8b; break;
    case UART_CFG_DATABITS_9: cfg.USART_WordLength = USART_WordLength_9b; break;
    default: return FALSE;
    }
    switch (stopbits) {
    case UART_CFG_STOPBITS_0_5: cfg.USART_StopBits = USART_StopBits_0_5; break;
    case UART_CFG_STOPBITS_1: cfg.USART_StopBits = USART_StopBits_1; break;
    case UART_CFG_STOPBITS_1_5: cfg.USART_StopBits = USART_StopBits_1_5; break;
    case UART_CFG_STOPBITS_2: cfg.USART_StopBits = USART_StopBits_2; break;
    default: return FALSE;
    }
    switch (parity) {
    case UART_CFG_PARITY_NONE: cfg.USART_Parity = USART_Parity_No; break;
    case UART_CFG_PARITY_EVEN: cfg.USART_Parity = USART_Parity_Even; break;
    case UART_CFG_PARITY_ODD: cfg.USART_Parity = USART_Parity_Odd; break;
    default: return FALSE;
    }
    switch (flowcontrol) {
    case UART_CFG_FLOWCONTROL_NONE: cfg.USART_HardwareFlowControl = USART_HardwareFlowControl_None; break;
    case UART_CFG_FLOWCONTROL_RTS: cfg.USART_HardwareFlowControl = USART_HardwareFlowControl_RTS; break;
    case UART_CFG_FLOWCONTROL_CTS: cfg.USART_HardwareFlowControl = USART_HardwareFlowControl_CTS; break;
    case UART_CFG_FLOWCONTROL_RTS_CTS: cfg.USART_HardwareFlowControl = USART_HardwareFlowControl_RTS_CTS; break;
    default: return FALSE;
    }
    cfg.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(UART_HW(uart), &cfg);

    // Enable USART interrupts
    USART_ITConfig(UART_HW(uart), USART_IT_TC, DISABLE);
    USART_ITConfig(UART_HW(uart), USART_IT_TXE, DISABLE);
    USART_ITConfig(UART_HW(uart), USART_IT_RXNE, ENABLE);
    USART_Cmd(UART_HW(uart), ENABLE);
  } else {
    USART_ITConfig(UART_HW(uart), USART_IT_TC, DISABLE);
    USART_ITConfig(UART_HW(uart), USART_IT_TXE, DISABLE);
    USART_ITConfig(UART_HW(uart), USART_IT_RXNE, DISABLE);
    USART_Cmd(UART_HW(uart), DISABLE);
  }
  return TRUE;
}
#endif

void UART_init() {
  memset(__uart_vec, 0, sizeof(__uart_vec));
  int uc = 0;

#ifdef CONFIG_UART1
  _UART(uc++)->hw = USART1;
  UART_config(_UART(uc-1), UART1_SPEED, UART_DATABITS_8, UART_STOPBITS_1,
      UART_PARITY_NONE, UART_FLOWCONTROL_NONE, TRUE);
  UART_TX_IRQ_OFF(_UART(uc-1));
#endif
#ifdef CONFIG_UART2
  _UART(uc++)->hw = USART2;
  UART_config(_UART(uc-1), UART2_SPEED, UART_DATABITS_8, UART_STOPBITS_1,
      UART_PARITY_NONE, UART_FLOWCONTROL_NONE, TRUE);
  UART_TX_IRQ_OFF(_UART(uc-1));
#endif
#ifdef CONFIG_UART3
  _UART(uc++)->hw = USART3;
  UART_config(_UART(uc-1), UART3_SPEED, UART_DATABITS_8, UART_STOPBITS_1,
      UART_PARITY_NONE, UART_FLOWCONTROL_NONE, TRUE);
  UART_TX_IRQ_OFF(_UART(uc-1));
#endif
#ifdef CONFIG_UART4
  _UART(uc++)->hw = UART4;
  UART_config(_UART(uc-1), UART4_SPEED, UART_DATABITS_8, UART_STOPBITS_1,
      UART_PARITY_NONE, UART_FLOWCONTROL_NONE, TRUE);
  UART_TX_IRQ_OFF(_UART(uc-1));
#endif
}

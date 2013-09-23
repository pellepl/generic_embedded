/*
 * usr_wifi232_driver.c
 *
 *  Created on: Aug 28, 2013
 *      Author: petera
 */

#include "usr_wifi232_driver.h"
#include "miniutils.h"
#include "io.h"
#include "taskq.h"

#define WIFI_BUF_SIZE 512

typedef enum {
  WIFI_DATA = 0,
  WIFI_ENTER_PLUS,
  WIFI_ENTER_A,
  WIFI_ECHO_COMMAND,
  WIFI_COMMAND_RES,
  WIFI_EXIT,
  WIFI_EXIT1,
  WIFI_EXIT2,
} wifi_cfg_state;

static struct {
  volatile bool config;
  u8_t enter_tries;
  volatile bool busy;
  wifi_cfg_state state;
  wifi_cfg_cmd cmd;
  void *data;
  wifi_cb cb;
  u32_t out_rix;
  u32_t out_wix;
  char out[WIFI_BUF_SIZE];
  u8_t cfg_cmd_linenbr;
  task_timer timeout_timer;
  task *timeout_task;
} wsta;


static void wifi_enter_config_tx() {
  IO_put_buf(IOWIFI, (u8_t*)"+++", 3);
  TASK_stop_timer(&wsta.timeout_timer);
  TASK_start_timer(wsta.timeout_task,
      &wsta.timeout_timer, 0, NULL,
      300, 0, "wifi_tmo");
  wsta.enter_tries++;
}

static void wifi_enter_config() {
  DBG(D_WIFI, D_DEBUG, "WIFI: enter config mode\n");
  wsta.config = TRUE;
  wsta.state = WIFI_ENTER_PLUS;
  wsta.enter_tries = 0;
  wifi_enter_config_tx();
}

static void wifi_exit_config(void) {
  wsta.state = WIFI_EXIT;
  DBG(D_WIFI, D_DEBUG, "WIFI: exit config mode\n");
  IO_put_buf(IOWIFI, (u8_t*)"at+entm\n", 8);
}

// emit command to usr wifi
static void wifi_do_command(void) {
  DBG(D_WIFI, D_DEBUG, "WIFI: command %i\n", wsta.cmd);

  switch (wsta.cmd) {
  case WIFI_SCAN:
    DBG(D_WIFI, D_DEBUG, "WIFI: < at+wscan\n");
    IO_put_buf(IOWIFI, (u8_t*)"at+wscan\n", 9);
    break;
  case WIFI_GET_WAN:
    DBG(D_WIFI, D_DEBUG, "WIFI: < at+wann\n");
    IO_put_buf(IOWIFI, (u8_t*)"at+wann\n", 8);
    break;
  case WIFI_GET_SSID:
    DBG(D_WIFI, D_DEBUG, "WIFI: < at+wsssid\n");
    IO_put_buf(IOWIFI, (u8_t*)"at+wsssid\n", 10);
    break;
  }
}

static bool wifi_read_netwaddr(u8_t nwaddr[4], cursor *c, strarg *arg) {
  if (strarg_next_delim(c, arg, ".")) {
    nwaddr[0] = arg->val;
  } else return FALSE;
  if (strarg_next_delim(c, arg, ".")) {
    nwaddr[1] = arg->val;
  } else return FALSE;
  if (strarg_next_delim(c, arg, ".")) {
    nwaddr[2] = arg->val;
  } else return FALSE;
  if (strarg_next_delim(c, arg, "., ")) {
    nwaddr[3] = arg->val;
  } else return FALSE;
  return TRUE;
}

// read command result from usr wifi
static void wifi_on_cmd_res(char *line, u32_t strlen, u8_t linenbr) {
  DBG(D_WIFI, D_DEBUG, "WIFI: cmd res line%i \"%s\"\n", linenbr, line);

  if (strlen >= 4 && strncmp("+ERR", line, 4) == 0) {
    DBG(D_WIFI, D_WARN, "WIFI: config err %s\n", line);

    wsta.cb(wsta.cmd, WIFI_ERR_CONFIG, 0, 0);
    wifi_exit_config();
    return;
  }

  if (strlen == 0 && wsta.cmd != WIFI_SCAN) {
    wifi_exit_config();
    return;
  }

  cursor c;
  strarg arg;
  bool parse_ok = FALSE;

  switch (wsta.cmd) {

  case WIFI_SCAN: {
    if (linenbr >= 2 && strlen > 0) {
      strarg_init(&c, line, strlen);
      do {
        wifi_ap *ap = (wifi_ap *)wsta.data;
        if (strarg_next_delim(&c, &arg, ",")) {
          ap->channel = arg.val;
        } else break;
        if (strarg_next_delim_str(&c, &arg, ",")) {
          strcpy(ap->ssid, arg.str);
        } else break;
        if (strarg_next_delim_str(&c, &arg, ",")) {
          strncpy(ap->mac, arg.str, arg.len);
        } else break;
        if (strarg_next_delim_str(&c, &arg, ",")) {
          strcpy(ap->encryption, arg.str);
        } else break;
        if (strarg_next_delim(&c, &arg, ",")) {
          ap->signal = arg.val;
        } else break;
        parse_ok = TRUE;
        wsta.cb(wsta.cmd, WIFI_OK, 0, ap);
      } while (0);
      if (!parse_ok) {
        wsta.cb(wsta.cmd, WIFI_ERR_PARSE, 0, 0);
      }
    } else if (linenbr > 2) {
      parse_ok = TRUE;
      wsta.cb(wsta.cmd, WIFI_END_OF_SCAN, 0, 0);
      wifi_exit_config();
    } else {
      // just pass this line
      parse_ok = TRUE;
    }
    break;
  }

  case WIFI_GET_WAN: {
    //+ok=STATIC,192.168.0.124,255.255.255.0,192.168.0.1
    if (strlen > 4) {
      strarg_init(&c, line+4, strlen-4);
      wifi_wan_setting *wan = (wifi_wan_setting *)wsta.data;
      do {
        if (strarg_next_delim_str(&c, &arg, ",")) {
          if (strcmp("STATIC", arg.str) == 0) {
            wan->method = WIFI_WAN_STATIC;
          } else if (strcmp("DHCP", arg.str) == 0) {
            wan->method = WIFI_WAN_DHCP;
          } else break;
        } else break;
        if (!wifi_read_netwaddr(wan->ip, &c, &arg)) break;
        if (!wifi_read_netwaddr(wan->netmask, &c, &arg)) break;
        if (!wifi_read_netwaddr(wan->gateway, &c, &arg)) break;

        parse_ok = TRUE;
        wsta.cb(wsta.cmd, WIFI_OK, 0, wan);
      } while (0);
    }
    if (!parse_ok) {
      wsta.cb(wsta.cmd, WIFI_ERR_PARSE, 0, 0);
    }

    break;
  }

  case WIFI_GET_SSID: {
    //+ok=springfield
    if (strlen > 4) {
      strarg_init(&c, line+4, strlen-4);
      char *ssid = (char *)wsta.data;
      do {
        if (strarg_next_str(&c, &arg)) {
          strcpy(ssid, arg.str);
        } else break;

        parse_ok = TRUE;
        wsta.cb(wsta.cmd, WIFI_OK, 0, ssid);
      } while (0);
    }
    if (!parse_ok) {
      wsta.cb(wsta.cmd, WIFI_ERR_PARSE, 0, 0);
    }

    break;
  }
  }
}

static void wifi_cfg_on_line(char *line, u32_t strlen) {
  if (wsta.state != WIFI_COMMAND_RES) {
    DBG(D_WIFI, D_DEBUG, "WIFI: > len:%4i \"%s\"\n", strlen, line);
  }
  if (wsta.state != WIFI_ENTER_PLUS) {
    TASK_stop_timer(&wsta.timeout_timer);
    TASK_start_timer(wsta.timeout_task,
        &wsta.timeout_timer, 0, NULL,
        3000, 0,
        "wifi_tmo");
  }

  switch (wsta.state) {
  case WIFI_ENTER_PLUS:
    if (strcmp("a", line)) {
      wsta.state = WIFI_ENTER_A;
      IO_put_char(IOWIFI, 'a');
    }
    break;
  case WIFI_ENTER_A:
    if (strcmp("+ok", line)) {
      wsta.state = WIFI_ECHO_COMMAND;
      wifi_do_command();
    }
    break;
  case WIFI_ECHO_COMMAND:
    wsta.cfg_cmd_linenbr = 0;
    wsta.state = WIFI_COMMAND_RES;
    break;
  case WIFI_COMMAND_RES:
    wifi_on_cmd_res(line, strlen, wsta.cfg_cmd_linenbr);
    wsta.cfg_cmd_linenbr++;
    break;
  case WIFI_EXIT:
    wsta.state = WIFI_EXIT1;
    break;
  case WIFI_EXIT1:
    wsta.state = WIFI_EXIT2;
    break;
  case WIFI_EXIT2:
    wsta.busy = FALSE;
    wsta.state = WIFI_DATA;
    DBG(D_WIFI, D_DEBUG, "WIFI: config exited\n");

    break;
  default:
    break;
  }
}

void wifi_task_on_line(u32_t rix, void* pwix) {
  u32_t wix = (u32_t)pwix;
  char buf[WIFI_BUF_SIZE/4];
  u32_t len;

  if (rix == wix) {
    len = 0;
    buf[0] = 0;
  } else if (rix < wix) {
    len = wix-rix;
    memcpy(buf, &wsta.out[rix], len);
    buf[len] = 0;
  } else {
    memcpy(buf, &wsta.out[rix], WIFI_BUF_SIZE-rix);
    memcpy(&buf[WIFI_BUF_SIZE-rix], &wsta.out[0], wix);
    len = wix + (WIFI_BUF_SIZE-rix);
    buf[len] = 0;
  }
  wifi_cfg_on_line(buf, len);
}

static void wifi_io_parse(u8_t c) {
  if (c == '\r') return;
  // after three +++, an 'a' is followed w/o carrige return
  if (c == '\n' || (wsta.state == WIFI_ENTER_PLUS && c == 'a')) {
    wsta.out[wsta.out_wix] = 0;
    task *t = TASK_create(wifi_task_on_line, 0);
    TASK_run(t, wsta.out_rix, (void*)(wsta.out_wix));
    wsta.out_wix++;
    if (wsta.out_wix >= WIFI_BUF_SIZE) {
      wsta.out_wix = 0;
    }
    wsta.out_rix = wsta.out_wix;
  } else {
    wsta.out[wsta.out_wix] = c;
    wsta.out_wix++;
    if (wsta.out_wix >= WIFI_BUF_SIZE) {
      wsta.out_wix = 0;
    }
  }
}

static void wifi_io_cb(u8_t io, void *arg, u16_t len) {
  if (wsta.config) {
    u8_t buf[256];
    u16_t ix;
    IO_get_buf(io, buf, len);
    for (ix = 0; ix < len; ix++) {
      wifi_io_parse(buf[ix]);
    }
  } else {
    // TODO data
  }
}

static void wifi_tmo(u32_t arg, void *argp) {
  bool tmo = FALSE;
  if (wsta.busy) {
    if (wsta.state == WIFI_ENTER_PLUS) {
      if (wsta.enter_tries >= 3) {
        tmo = TRUE;
      } else {
        wifi_enter_config_tx();
      }
    } else {
      tmo = TRUE;
    }
  }
  if (tmo) {
    DBG(D_WIFI, D_WARN, "WIFI: timeout\n");
    wsta.busy = FALSE;
    if (wsta.cb) {
      wsta.cb(wsta.cmd, WIFI_ERR_TIMEOUT, 0, 0);
    }
  }
}

//////////////////////////////// api

int WIFI_scan(wifi_ap *ap) {
  if (!WIFI_is_ready()) {
    return WIFI_ERR_NOT_READY;
  }
  if (wsta.busy) {
    return WIFI_ERR_BUSY;
  }
  wsta.busy = TRUE;
  wsta.data = ap;
  wsta.cmd = WIFI_SCAN;

  wifi_enter_config();

  return WIFI_OK;
}

int WIFI_get_wan(wifi_wan_setting *wan) {
  if (!WIFI_is_ready()) {
    return WIFI_ERR_NOT_READY;
  }
  if (wsta.busy) {
    return WIFI_ERR_BUSY;
  }
  wsta.busy = TRUE;
  wsta.data = wan;
  wsta.cmd = WIFI_GET_WAN;

  wifi_enter_config();

  return WIFI_OK;
}

int WIFI_get_ssid(char *ssid) {
  if (!WIFI_is_ready()) {
    return WIFI_ERR_NOT_READY;
  }
  if (wsta.busy) {
    return WIFI_ERR_BUSY;
  }
  wsta.busy = TRUE;
  wsta.data = ssid;
  wsta.cmd = WIFI_GET_SSID;

  wifi_enter_config();

  return WIFI_OK;
}


//////////////////////////////// HW


bool WIFI_is_ready(void) {
  bool ready = GPIO_read(WIFI_GPIO_PORT, WIFI_GPIO_READY_PIN) == 0;
  return ready;
}

bool WIFI_is_link(void) {
  bool link = GPIO_read(WIFI_GPIO_PORT, WIFI_GPIO_LINK_PIN) == 0;
  return link;
}

void WIFI_reset(void) {
  GPIO_set(WIFI_GPIO_PORT, 0, WIFI_GPIO_RESET_PIN);
  SYS_hardsleep_ms(400);
  GPIO_set(WIFI_GPIO_PORT, WIFI_GPIO_RESET_PIN, 0);
  wsta.busy = FALSE;
}

void WIFI_factory_reset(void) {
  GPIO_set(WIFI_GPIO_PORT, 0, WIFI_GPIO_RELOAD_PIN);
  SYS_hardsleep_ms(1200);
  GPIO_set(WIFI_GPIO_PORT, WIFI_GPIO_RELOAD_PIN, 0);
  wsta.busy = FALSE;
}

void WIFI_state(void) {
  bool ready = WIFI_is_ready();
  bool link = WIFI_is_link();
  print("WIFI ready: %s\n", ready ? "YES":"NO");
  print("WIFI link : %s\n", link ? "YES":"NO");
}


//////////////////////////////// init

void WIFI_init(wifi_cb cb) {
  GPIO_set(WIFI_GPIO_PORT, WIFI_GPIO_RESET_PIN, 0);
  GPIO_set(WIFI_GPIO_PORT, WIFI_GPIO_RELOAD_PIN, 0);

  UART_config(_UART(WIFI_UART), 0,0,0,0,0, FALSE);

  IO_assure_tx(IOWIFI, TRUE);

  memset(&wsta, 0, sizeof(wsta));
  wsta.cb = cb;

  UART_config(_UART(WIFI_UART), WIFI_UART_BAUD, UART_DATABITS_8, UART_STOPBITS_1,
      UART_PARITY_NONE, UART_FLOWCONTROL_NONE, TRUE);

  IO_set_callback(IOWIFI, wifi_io_cb, 0);

  wsta.timeout_task = TASK_create(wifi_tmo, TASK_STATIC);
}
/*
AT+WMODE
+ok=STA

AT+WSKEY
+ok=WPA2PSK,AES,<*******>

AT+WSSSID
+ok=springfield

AT+WANN
+ok=STATIC,192.168.0.124,255.255.255.0,192.168.0.1
+ok=DHCP,0.0.0.0,255.255.255.0,192.168.0.1

AT+LANGW
+ok=192.168.0.1

AT+NETP
+ok=UDP,Client,51966,192.168.0.204

 *
 */

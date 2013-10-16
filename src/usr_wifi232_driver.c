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

#define WIFI_CONFIG_START   WIFI_CONFIG_MODE
#define WIFI_CONFIG_MODE    0
#define WIFI_CONFIG_SSID    1
#define WIFI_CONFIG_ENCR    2
#define WIFI_CONFIG_WAN     3
#define WIFI_CONFIG_GATEWAY 4
#define WIFI_CONFIG_CTRL    5
#define WIFI_CONFIG_END     255

static struct {
  wifi_cb cfg_cb;
  wifi_data_cb data_cb;
  wifi_data_timeout_cb data_tmo_cb;

  volatile bool config;

  u8_t enter_tries;

  volatile bool busy;
  wifi_cfg_state cfg_state;
  u8_t cfg_sub_state;
  wifi_cfg_cmd cfg_cmd;

  void *cfg_result;
  u32_t cfg_in_rix;
  u32_t cfg_in_wix;
  char cfg_in[WIFI_BUF_SIZE];
  u8_t cfg_cmd_linenbr;

  task_timer cfg_timeout_timer;
  task *cfg_timeout_task;

  volatile bool handling_input;

  ringbuf rx_data_rb;
  u8_t rx_data_buf[CONFIG_WIFI_RX_MAX_LEN];

  u8_t delim_mask;
  u16_t delim_char;
  u32_t delim_length;
  u32_t delim_timeout;

  task_timer data_timer;
  task *data_timer_task;
  volatile bool timer_started;
  volatile bool timer_triggered;

  time data_last_time;
  time data_silence_timeout;
} wsta;

///////////////////////////////////////////

static void wifi_data_handle_input(u8_t io);

/////////////////////////////////////////// config

static void wifi_cfg_tx_enter() {
  IO_put_buf(IOWIFI, (u8_t*)"+++", 3);
  TASK_stop_timer(&wsta.cfg_timeout_timer);
  TASK_start_timer(wsta.cfg_timeout_task,
      &wsta.cfg_timeout_timer, 0, NULL,
      300, 0, "wifi_tmo");
  wsta.enter_tries++;
}

static void wifi_cfg_enter() {
  DBG(D_WIFI, D_DEBUG, "WIFI: enter config mode\n");
  wsta.config = TRUE;
  wsta.cfg_state = WIFI_ENTER_PLUS;
  wsta.enter_tries = 0;
  wifi_cfg_tx_enter();
}

static void wifi_cfg_exit(void) {
  wsta.cfg_state = WIFI_EXIT;
  DBG(D_WIFI, D_DEBUG, "WIFI: exit config mode\n");
  IO_put_buf(IOWIFI, (u8_t*)"at+entm\n", 8);
}

// emit command to usr wifi
static void wifi_cfg_do_command(void) {
  DBG(D_WIFI, D_DEBUG, "WIFI: command %i\n", wsta.cfg_cmd);

  switch (wsta.cfg_cmd) {
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

  case WIFI_SET_CONFIG: {
    wifi_config *c = (wifi_config *)wsta.cfg_result;
    switch (wsta.cfg_sub_state) {
    case WIFI_CONFIG_MODE:
      //AT+WMODE
      //+ok=STA
      DBG(D_WIFI, D_DEBUG, "WIFI: < at+wmode=STA\n");
      ioprint(IOWIFI, "at+wmode=STA\n");
      break;
    case WIFI_CONFIG_SSID:
      //AT+WSSSID
      //+ok=springfield
      DBG(D_WIFI, D_DEBUG, "WIFI: < at+wsssid=x\n");
      ioprint(IOWIFI, "at+wsssid=%s\n", c->ssid);
      break;
    case WIFI_CONFIG_ENCR:
      DBG(D_WIFI, D_DEBUG, "WIFI: < at+wskey=x,y,z\n");
      //AT+WSKEY
      //+ok=WPA2PSK,AES,<*******>
      ioprint(IOWIFI, "at+wskey=%s,%s\n",
          (u8_t*)c->encryption,
          (u8_t*)c->password);
      break;
    case WIFI_CONFIG_WAN:
      //AT+WANN
      //+ok=STATIC,192.168.0.124,255.255.255.0,192.168.0.1
      //+ok=DHCP,0.0.0.0,255.255.255.0,192.168.0.1
      DBG(D_WIFI, D_DEBUG, "WIFI: < at+wann=x,y,z,w\n");
      ioprint(IOWIFI, "at+wann=%s,%i.%i.%i.%i,%i.%i.%i.%i,%i.%i.%i.%i\n",
          c->wan.method == WIFI_WAN_STATIC ? "STATIC":"DHCP",
          c->wan.ip[0], c->wan.ip[1], c->wan.ip[2], c->wan.ip[3],
          c->wan.netmask[0], c->wan.netmask[1], c->wan.netmask[2], c->wan.netmask[3],
          c->wan.gateway[0], c->wan.gateway[1], c->wan.gateway[2], c->wan.gateway[3]);
      break;
    case WIFI_CONFIG_GATEWAY:
      //AT+LANGW
      //+ok=192.168.0.1
      DBG(D_WIFI, D_DEBUG, "WIFI: < at+langw=x\n");
      ioprint(IOWIFI, "at+langw=%i.%i.%i.%i\n",
          c->gateway[0], c->gateway[1], c->gateway[2], c->gateway[3]);
      break;
    case WIFI_CONFIG_CTRL:
      //AT+NETP
      //+ok=UDP,Client,51966,192.168.0.204
      DBG(D_WIFI, D_DEBUG, "WIFI: < at+netp=x,y,z\n");
      ioprint(IOWIFI, "at+netp=%s,%s,%i,%i.%i.%i.%i\n",
          c->protocol == WIFI_COMM_PROTO_TCP ? "TCP" : "UDP",
          c->type == WIFI_TYPE_SERVER ? "Server":"Client",
          c->port,
          c->server[0], c->server[1], c->server[2], c->server[3]);
      break;
    default:
      //AT+Z
      DBG(D_WIFI, D_DEBUG, "WIFI: < at+z\n");
      ioprint(IOWIFI, "at+z\n");
      break;
    } // substate
    } // case set_config
    break;
  } // cmd
}

static bool wifi_cfg_read_netwaddr(u8_t nwaddr[4], cursor *c, strarg *arg) {
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
static void wifi_cfg_on_cmd_res(char *line, u32_t strlen, u8_t linenbr) {
  DBG(D_WIFI, D_DEBUG, "WIFI: cmd res line%i \"%s\"\n", linenbr, line);

  if (strlen >= 4 && strncmp("+ERR", line, 4) == 0) {
    DBG(D_WIFI, D_WARN, "WIFI: config err %s\n", line);

    wsta.cfg_cb(wsta.cfg_cmd, WIFI_ERR_CONFIG, 0, 0);
    wifi_cfg_exit();
    return;
  }

  if (strlen == 0 &&
      (wsta.cfg_cmd != WIFI_SCAN &&
       wsta.cfg_cmd != WIFI_SET_CONFIG)
      ) {
    wifi_cfg_exit();
    return;
  }

  cursor c;
  strarg arg;
  bool parse_ok = FALSE;

  switch (wsta.cfg_cmd) {

  case WIFI_SCAN: {
    if (linenbr >= 2 && strlen > 0) {
      strarg_init(&c, line, strlen);
      do {
        wifi_ap *ap = (wifi_ap *)wsta.cfg_result;
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
        wsta.cfg_cb(wsta.cfg_cmd, WIFI_OK, 0, ap);
      } while (0);
      if (!parse_ok) {
        wsta.cfg_cb(wsta.cfg_cmd, WIFI_ERR_PARSE, 0, 0);
      }
    } else if (linenbr > 2) {
      parse_ok = TRUE;
      wsta.cfg_cb(wsta.cfg_cmd, WIFI_END_OF_SCAN, 0, 0);
      wifi_cfg_exit();
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
      wifi_wan_setting *wan = (wifi_wan_setting *)wsta.cfg_result;
      do {
        if (strarg_next_delim_str(&c, &arg, ",")) {
          if (strcmp("STATIC", arg.str) == 0) {
            wan->method = WIFI_WAN_STATIC;
          } else if (strcmp("DHCP", arg.str) == 0) {
            wan->method = WIFI_WAN_DHCP;
          } else break;
        } else break;
        if (!wifi_cfg_read_netwaddr(wan->ip, &c, &arg)) break;
        if (!wifi_cfg_read_netwaddr(wan->netmask, &c, &arg)) break;
        if (!wifi_cfg_read_netwaddr(wan->gateway, &c, &arg)) break;

        parse_ok = TRUE;
        wsta.cfg_cb(wsta.cfg_cmd, WIFI_OK, 0, wan);
      } while (0);
    }
    if (!parse_ok) {
      wsta.cfg_cb(wsta.cfg_cmd, WIFI_ERR_PARSE, 0, 0);
    }

    break;
  }

  case WIFI_GET_SSID: {
    //+ok=springfield
    if (strlen > 4) {
      strarg_init(&c, line+4, strlen-4);
      char *ssid = (char *)wsta.cfg_result;
      do {
        if (strarg_next_str(&c, &arg)) {
          strcpy(ssid, arg.str);
        } else break;

        parse_ok = TRUE;
        wsta.cfg_cb(wsta.cfg_cmd, WIFI_OK, 0, ssid);
      } while (0);
    }
    if (!parse_ok) {
      wsta.cfg_cb(wsta.cfg_cmd, WIFI_ERR_PARSE, 0, 0);
    }

    break;
  }

  case WIFI_SET_CONFIG: {
    wsta.cfg_sub_state++;
    wsta.cfg_state = WIFI_ENTER_A;
    break;
  }
  }
}

static void wifi_cfg_on_line(char *line, u32_t strlen) {
  if (wsta.cfg_state != WIFI_COMMAND_RES) {
    DBG(D_WIFI, D_DEBUG, "WIFI: > len:%4i \"%s\"\n", strlen, line);
  }
  if (wsta.cfg_state != WIFI_ENTER_PLUS) {
    TASK_stop_timer(&wsta.cfg_timeout_timer);
    TASK_start_timer(wsta.cfg_timeout_task,
        &wsta.cfg_timeout_timer, 0, NULL,
        3000, 0,
        "wifi_tmo");
  }

  switch (wsta.cfg_state) {
  case WIFI_ENTER_PLUS:
    if (strcmp("a", line)) {
      wsta.cfg_state = WIFI_ENTER_A;
      IO_put_char(IOWIFI, 'a');
    }
    break;
  case WIFI_ENTER_A:
    if (strcmp("+ok", line)) {
      wsta.cfg_state = WIFI_ECHO_COMMAND;
      wifi_cfg_do_command();
    }
    break;
  case WIFI_ECHO_COMMAND:
    wsta.cfg_cmd_linenbr = 0;
    wsta.cfg_state = WIFI_COMMAND_RES;
    if (strlen >= 4 && strncmp("AT+Z", line, 4) == 0) {
      DBG(D_WIFI, D_INFO, "WIFI: sw reset\n");
      wsta.cfg_cb(wsta.cfg_cmd, WIFI_SW_RESET, 0, 0);
      WIFI_reset(FALSE);
      return;
    }
    break;
  case WIFI_COMMAND_RES:
    wifi_cfg_on_cmd_res(line, strlen, wsta.cfg_cmd_linenbr);
    wsta.cfg_cmd_linenbr++;
    break;
  case WIFI_EXIT:
    wsta.cfg_state = WIFI_EXIT1;
    break;
  case WIFI_EXIT1:
    wsta.cfg_state = WIFI_EXIT2;
    break;
  case WIFI_EXIT2:
    wsta.busy = FALSE;
    wsta.cfg_state = WIFI_DATA;
    DBG(D_WIFI, D_DEBUG, "WIFI: config exited\n");

    break;
  default:
    break;
  }
}

void wifi_cfg_task_on_line(u32_t rix, void* pwix) {
  u32_t wix = (u32_t)pwix;
  char buf[WIFI_BUF_SIZE/4];
  u32_t len;

  if (rix == wix) {
    len = 0;
    buf[0] = 0;
  } else if (rix < wix) {
    len = wix-rix;
    memcpy(buf, &wsta.cfg_in[rix], len);
    buf[len] = 0;
  } else {
    memcpy(buf, &wsta.cfg_in[rix], WIFI_BUF_SIZE-rix);
    memcpy(&buf[WIFI_BUF_SIZE-rix], &wsta.cfg_in[0], wix);
    len = wix + (WIFI_BUF_SIZE-rix);
    buf[len] = 0;
  }
  wifi_cfg_on_line(buf, len);
}

static void wifi_cfg_io_parse(u8_t c) {
  if (c == '\r') return;
  // after three +++, an 'a' is followed w/o carrige return
  if (c == '\n' || (wsta.cfg_state == WIFI_ENTER_PLUS && c == 'a')) {
    wsta.cfg_in[wsta.cfg_in_wix] = 0;
    task *t = TASK_create(wifi_cfg_task_on_line, 0);
    ASSERT(t);
    TASK_run(t, wsta.cfg_in_rix, (void*)(wsta.cfg_in_wix));
    wsta.cfg_in_wix++;
    if (wsta.cfg_in_wix >= WIFI_BUF_SIZE) {
      wsta.cfg_in_wix = 0;
    }
    wsta.cfg_in_rix = wsta.cfg_in_wix;
  } else {
    wsta.cfg_in[wsta.cfg_in_wix] = c;
    wsta.cfg_in_wix++;
    if (wsta.cfg_in_wix >= WIFI_BUF_SIZE) {
      wsta.cfg_in_wix = 0;
    }
  }
}



static void wifi_cfg_tmo(u32_t arg, void *argp) {
  bool tmo = FALSE;
  if (wsta.busy) {
    if (wsta.cfg_state == WIFI_ENTER_PLUS) {
      if (wsta.enter_tries >= 3) {
        tmo = TRUE;
      } else {
        wifi_cfg_tx_enter();
      }
    } else {
      tmo = TRUE;
    }
  }
  if (tmo) {
    DBG(D_WIFI, D_WARN, "WIFI: timeout\n");
    wsta.busy = FALSE;
    if (wsta.cfg_cb) {
      wsta.cfg_cb(wsta.cfg_cmd, WIFI_ERR_TIMEOUT, 0, 0);
    }
  }
}


// called via uart, len will always be 1
static void wifi_io_cb(u8_t io, void *arg, u16_t len) {
  if (wsta.config) {
    // configuration input
    u8_t buf[8];
    u16_t ix;
    IO_get_buf(io, buf, MIN(sizeof(buf), len));
    for (ix = 0; ix < len; ix++) {
      wifi_cfg_io_parse(buf[ix]);
    }
  } else {
    // data input
    wifi_data_handle_input(io);
  }
}

//////////////////////////////// data handling internals

static void wifi_data_task_f(u32_t io, void *vrb) {
  wsta.handling_input = FALSE;
  if (wsta.data_cb) wsta.data_cb(io, (ringbuf *)vrb);
}

static void wifi_data_timer_task_f(u32_t io, void *vrb) {
  if (wsta.timer_triggered) {
    wsta.handling_input = FALSE;
    wsta.timer_triggered = FALSE;
    if (wsta.data_cb) wsta.data_cb(io, (ringbuf *)vrb);
  } else {
    if (SYS_get_time_ms() - wsta.data_last_time > wsta.data_silence_timeout) {
      wsta.timer_started = FALSE;
      TASK_stop_timer(&wsta.data_timer);
      if (wsta.data_tmo_cb) wsta.data_tmo_cb(IOWIFI);
    }
  }
}

static void wifi_data_handle_input(u8_t io) {
  s32_t c = IO_get_char(io);
  IO_put_char(IODBG, c);
  s32_t res = ringbuf_putc(&wsta.rx_data_rb, c);
  u32_t ringbuf_avail = ringbuf_available(&wsta.rx_data_rb);

  bool buffer_capacity_alert =
      (res == RB_ERR_FULL)
      || (ringbuf_avail > 3*CONFIG_WIFI_RX_MAX_LEN/4);
  if (res == RB_OK || buffer_capacity_alert) {
    bool handle = !wsta.handling_input && (
        buffer_capacity_alert ||
        (c == wsta.delim_char) ||
        ((wsta.delim_mask & WIFI_DELIM_LENGTH) && ringbuf_avail >= wsta.delim_length));
    if (handle) {
      // handle by error, buffer semi-full or static delimiting by length or char
      wsta.handling_input = TRUE;
      task *t = TASK_create(wifi_data_task_f, 0);
      ASSERT(t);
      TASK_run(t, IOWIFI, &wsta.rx_data_rb);
    } else if ((wsta.delim_mask & WIFI_DELIM_TIME)) {
      // got data, start timer
      wsta.timer_triggered = TRUE;
    }
  }

  wsta.data_last_time = SYS_get_time_ms();
  if (!wsta.timer_started) {
    wsta.timer_started = TRUE;
    TASK_start_timer(wsta.data_timer_task, &wsta.data_timer, IOWIFI, &wsta.rx_data_rb,
        wsta.delim_timeout, wsta.delim_timeout, "wifi_tim");
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
  wsta.cfg_result = ap;
  wsta.cfg_cmd = WIFI_SCAN;

  wifi_cfg_enter();

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
  wsta.cfg_result = wan;
  wsta.cfg_cmd = WIFI_GET_WAN;

  wifi_cfg_enter();

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
  wsta.cfg_result = ssid;
  wsta.cfg_cmd = WIFI_GET_SSID;

  wifi_cfg_enter();

  return WIFI_OK;
}

int WIFI_set_config(wifi_config *config) {
  if (!WIFI_is_ready()) {
    return WIFI_ERR_NOT_READY;
  }
  if (wsta.busy) {
    return WIFI_ERR_BUSY;
  }
  wsta.busy = TRUE;
  wsta.cfg_result = config;
  wsta.cfg_cmd = WIFI_SET_CONFIG;
  wsta.cfg_sub_state = WIFI_CONFIG_START;

  wifi_cfg_enter();

  return WIFI_OK;
}

void WIFI_set_data_delimiter(u8_t delim_mask, u8_t delim_char, u32_t delim_len, u32_t delim_ms) {
  wsta.delim_mask = delim_mask;

  if (delim_mask & WIFI_DELIM_CHAR) {
    wsta.delim_char = delim_char;
  } else {
    wsta.delim_char = 0xee00; // no u8_t will ever match this
  }

  if (delim_mask & WIFI_DELIM_LENGTH) {
    wsta.delim_length = delim_len;
    // got enough data for callback already?
    if (ringbuf_available(&wsta.rx_data_rb) >= delim_len) {
      if (!wsta.handling_input) {
        wsta.handling_input = TRUE;
        task *t = TASK_create(wifi_data_task_f, 0);
        ASSERT(t);
        TASK_run(t, IOWIFI, &wsta.rx_data_rb);
      }
    }
  }

  if (delim_mask & WIFI_DELIM_TIME) {
    wsta.timer_triggered = FALSE;
    wsta.delim_timeout = delim_ms;
    if (wsta.timer_started) {
      wsta.timer_triggered = FALSE;
      TASK_stop_timer(&wsta.data_timer);
    }
    wsta.timer_started = TRUE;
    TASK_start_timer(wsta.data_timer_task, &wsta.data_timer, IOWIFI, &wsta.rx_data_rb,
        wsta.delim_timeout, wsta.delim_timeout, "wifi_tim");
  } else {
    if (wsta.timer_started) {
      wsta.timer_started = FALSE;
      wsta.timer_triggered = FALSE;
      TASK_stop_timer(&wsta.data_timer);
    }
  }
}

void WIFI_set_data_silence_timeout(time timeout) {
  wsta.data_silence_timeout = timeout;
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

void WIFI_reset(bool hw) {
  TASK_stop_timer(&wsta.cfg_timeout_timer);
  if (hw) {
    GPIO_set(WIFI_GPIO_PORT, 0, WIFI_GPIO_RESET_PIN);
    SYS_hardsleep_ms(400);
    GPIO_set(WIFI_GPIO_PORT, WIFI_GPIO_RESET_PIN, 0);
  }
  wsta.busy = FALSE;
  wsta.cfg_cmd_linenbr = 0;
  wsta.config = FALSE;
  wsta.enter_tries = 0;
  wsta.cfg_in_rix = 0;
  wsta.cfg_in_wix = 0;
  wsta.cfg_state = 0;
  wsta.cfg_sub_state = 0;
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

void WIFI_init(wifi_cb cfg_cb, wifi_data_cb data_cb, wifi_data_timeout_cb data_tmo_cb) {
  GPIO_set(WIFI_GPIO_PORT, WIFI_GPIO_RESET_PIN, 0);
  GPIO_set(WIFI_GPIO_PORT, WIFI_GPIO_RELOAD_PIN, 0);

  UART_config(_UART(WIFI_UART), 0,0,0,0,0, FALSE);

  IO_assure_tx(IOWIFI, TRUE);

  memset(&wsta, 0, sizeof(wsta));

  wsta.cfg_cb = cfg_cb;
  wsta.data_cb = data_cb;
  wsta.data_tmo_cb = data_tmo_cb;

  wsta.cfg_timeout_task = TASK_create(wifi_cfg_tmo, TASK_STATIC);

  UART_config(_UART(WIFI_UART), WIFI_UART_BAUD, UART_DATABITS_8, UART_STOPBITS_1,
      UART_PARITY_NONE, UART_FLOWCONTROL_NONE, TRUE);

  ringbuf_init(&wsta.rx_data_rb, wsta.rx_data_buf, CONFIG_WIFI_RX_MAX_LEN);
  wsta.data_timer_task = TASK_create(wifi_data_timer_task_f, TASK_STATIC);
  ASSERT(wsta.data_timer_task);

  wsta.delim_mask = WIFI_DELIM_LENGTH;
  wsta.delim_length = 1;

  IO_set_callback(IOWIFI, wifi_io_cb, 0);

  wsta.data_silence_timeout = 10000;

  ASSERT(wsta.cfg_timeout_task);
}

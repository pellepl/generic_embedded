/*
 * usr_wifi232_driver.h
 *
 *  Created on: Aug 28, 2013
 *      Author: petera
 */

#ifndef USR_WIFI232_DRIVER_H_
#define USR_WIFI232_DRIVER_H_

#include "system.h"

#define WIFI_OK             0
#define WIFI_ERR_NOT_READY  -1
#define WIFI_ERR_TIMEOUT    -2
#define WIFI_ERR_BUSY       -3
#define WIFI_ERR_PARSE      -4
#define WIFI_ERR_CONFIG     -5

#define WIFI_END_OF_SCAN    1

typedef enum {
  WIFI_SCAN = 0,
  WIFI_GET_WAN,
  WIFI_GET_SSID,
} wifi_cfg_cmd;

typedef struct {
  u8_t channel;
  char ssid[64];
  char mac[6*3];
  char encryption[32];
  u8_t signal;
} wifi_ap;

typedef enum {
  WIFI_WAN_STATIC = 0,
  WIFI_WAN_DHCP
} wifi_wan_method;

typedef struct {
  wifi_wan_method method;
  u8_t ip[4];
  u8_t netmask[4];
  u8_t gateway[4];
} wifi_wan_setting;

typedef void (*wifi_cb)(wifi_cfg_cmd cmd, int res, u32_t arg, void *data);

void WIFI_init(wifi_cb cb);
void WIFI_reset(void);
void WIFI_factory_reset(void);
void WIFI_state(void);
bool WIFI_is_ready(void);
bool WIFI_is_link(void);
int WIFI_scan(wifi_ap *ap);
int WIFI_get_wan(wifi_wan_setting *wan);
int WIFI_get_ssid(char *c);


#endif /* USR_WIFI232_DRIVER_H_ */

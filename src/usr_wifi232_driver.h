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
#define WIFI_SW_RESET       -999

#define WIFI_END_OF_SCAN    1

typedef enum {
  WIFI_SCAN = 0,
  WIFI_GET_WAN,
  WIFI_GET_SSID,
  WIFI_SET_CONFIG,
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

typedef enum {
  WIFI_COMM_PROTO_TCP = 0,
  WIFI_COMM_PROTO_UDP
} wifi_comm_protocol;

typedef enum {
  WIFI_TYPE_SERVER = 0,
  WIFI_TYPE_CLIENT
} wifi_type;

typedef struct {
  wifi_wan_method method;
  u8_t ip[4];
  u8_t netmask[4];
  u8_t gateway[4];
} wifi_wan_setting;

typedef struct {
  // wan
  wifi_wan_setting wan;
  u8_t gateway[4];
  //wifi
  char ssid[64];
  char password[64];
  char encryption[32];
  // control
  wifi_type type;
  wifi_comm_protocol protocol;
  u16_t port;
  u8_t server[4];
} wifi_config;

typedef void (*wifi_cb)(wifi_cfg_cmd cmd, int res, u32_t arg, void *data);

typedef void (*wifi_data_cb)(u8_t io, u16_t len);

void WIFI_init(wifi_cb cb, wifi_data_cb dcb);
// Resets wifi block. If hw is true, a hardware reset is also perfomed on module.
void WIFI_reset(bool hw);
// Resets wifi module to default settings.
void WIFI_factory_reset(void);
// Queries wifi module state.
void WIFI_state(void);
// Returns if wifi module is booted and ready.
bool WIFI_is_ready(void);
// Returns if wifi module's uplink is established.
bool WIFI_is_link(void);
// Starts an AP scan, result is reported in callback function.
int WIFI_scan(wifi_ap *ap);
// Queries wan settings, result is reported in callback function.
int WIFI_get_wan(wifi_wan_setting *wan);
// Queries SSID, result is reported in callback function.
int WIFI_get_ssid(char *c);
// Configures wifi module. Successful configuration is ended by a reset which
// is reported in callback.
int WIFI_set_config(wifi_config *config);


#endif /* USR_WIFI232_DRIVER_H_ */

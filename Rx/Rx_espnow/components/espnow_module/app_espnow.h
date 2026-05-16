#ifndef APP_ESPNOW_H
#define APP_ESPNOW_H


#include "esp_mac.h"
#include "esp_now.h"


#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern uint8_t led_trigger;
void espnow_init(void);
void send_cb(const esp_now_send_info_t *info, esp_now_send_status_t status);
void recv_cb(const esp_now_recv_info_t *info, const uint8_t *data,int len);
void update_node_directory(uint8_t id, const uint8_t *mac);
bool get_mac_by_node_id(uint8_t id, uint8_t *out_mac);
#endif
#ifndef WIFI_INIT_H
#define WIFI_INIT_H

#include "esp_wifi.h"
#include "esp_netif.h"
#include "mqtt_client.h"

#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data);
void wifi_init(void);


#endif
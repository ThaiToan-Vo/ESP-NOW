#ifndef APP_MQTT_H
#define APP_MQTT_H



#include "mqtt_client.h"
#include "cJSON.h"

#include "esp_event.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LED_PIN GPIO_NUM_2
extern uint32_t MQTT_CONNEECTED;
extern esp_mqtt_client_handle_t client;
void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
void mqtt_app_start(void);
void gpio_init(void);
#endif
#ifndef TASK_MQTT_SEND_H
#define TASK_MQTT_SEND_H

#include "esp_log.h"
#include "mqtt_client.h"
#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"   
#include "freertos/queue.h"


void app_task_mqtt_send (void);

#endif // TASK_MQTT_SEND_H
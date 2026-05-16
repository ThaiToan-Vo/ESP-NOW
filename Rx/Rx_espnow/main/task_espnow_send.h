#ifndef TASK_ESPNOW_SEND_H
#define TASK_ESPNOW_SEND_H

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"   
#include "freertos/queue.h"

void app_task_espnow_send (void);

#endif // TASK_ESPNOW_SEND_H
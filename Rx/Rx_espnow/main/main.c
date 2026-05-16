#include <stdio.h>
#include "string.h"


#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "app_mqtt.h"
#include "app_espnow.h" 
#include "wifi_init.h"

#include "struct_common.h"
#include "task_espnow_send.h"
#include "task_mqtt_send.h"

QueueHandle_t mqtt_to_espnow_queue = NULL;
QueueHandle_t espnow_to_mqtt_queue = NULL;

static const char *TAG = "MQTT_EXAMPLE";


// declare functions
void send_cb(const esp_now_send_info_t *info, esp_now_send_status_t status);
void recv_cb(const esp_now_recv_info_t *info, const uint8_t *data, int len);
void mqtt_app_start(void);  // hàm này được gọi trong event IP của wifi init


void Publisher_Task(void *params)
{
  while (true)
  {
    if(MQTT_CONNEECTED)
    {
        esp_mqtt_client_publish(client, "/topic/test3", "Helllo World", 0, 0, 0);
        vTaskDelay(15000 / portTICK_PERIOD_MS);
    }
    else 
    {
        // NẾU CHƯA KẾT NỐI: Phải nghỉ để nhường CPU cho WiFi/MQTT xử lý việc kết nối!
        vTaskDelay(pdMS_TO_TICKS(1000)); 
    }
  }
}


void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
        {
            nvs_flash_erase();
            ret = nvs_flash_init();
        }
    mqtt_to_espnow_queue = xQueueCreate(10, sizeof(node_ctrl_t));
    espnow_to_mqtt_queue = xQueueCreate(15, sizeof(power_data_t));
    app_task_espnow_send();
    app_task_mqtt_send();
    
    gpio_init();
    wifi_init();
    espnow_init();
    xTaskCreate(Publisher_Task, "Publisher_Task", 1024 * 5, NULL, 5, NULL);
    
    while (1)
    {
        if (led_trigger)
        {
            gpio_set_level(LED_PIN, 1);
            vTaskDelay(pdMS_TO_TICKS(2000));
            gpio_set_level(LED_PIN, 0);
            led_trigger = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

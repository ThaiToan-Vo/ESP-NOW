#include "task_espnow_send.h"
#include "struct_common.h"
#include "app_espnow.h"

static const char *TAG = "ESP-NOW_SEND_TASK";

void task_espnow_send(void *pvParameters) 
{
    node_ctrl_t ctrl_data;
    while (1) 
    {
        // 1. Nhận dữ liệu từ Queue được gửi từ app mqtt
        if (xQueueReceive(mqtt_to_espnow_queue, &ctrl_data, portMAX_DELAY) == pdTRUE) 
        {
            ESP_LOGI("ESP-NOW_SEND", "Nhận được lệnh từ MQTT cho Node %d", ctrl_data.node_id);
        }
        
        // 2. Tìm MAC của Node dựa trên node_id 
        uint8_t target_mac[6];
        if (get_mac_by_node_id(ctrl_data.node_id, target_mac)) 
        {
            // 3. Gửi qua ESP-NOW thông qua mac đã tìm được
            esp_err_t ret = esp_now_send(target_mac, (uint8_t *)&ctrl_data, sizeof(ctrl_data));
            if (ret == ESP_OK) 
            {
                ESP_LOGI("ESP-NOW_SEND", "Đã gửi lệnh xuống Node %d", ctrl_data.node_id);
            }
        } 
        
        else 
        {
            ESP_LOGW("ESP-NOW_SEND", "Không tìm thấy MAC cho Node ID: %d", ctrl_data.node_id);
        }
    }
}

void app_task_espnow_send (void)
{
    xTaskCreate(task_espnow_send, "ESP-NOW_Send_Task", 3072, NULL, 5, NULL);
}
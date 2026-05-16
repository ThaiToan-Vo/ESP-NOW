#include "task_mqtt_send.h"
#include "struct_common.h"
#include "app_mqtt.h"
static const char *TAG = "MQTT_SEND_TASK";


void task_mqtt_send (void *pvParameters) 
{
    power_data_t power_data ;
    char topic[32];
    while (1) 
    {
        // 1. Nhận dữ liệu từ Queue espnow để gửi lên mqtt
        if (xQueueReceive(espnow_to_mqtt_queue, &power_data, portMAX_DELAY) == pdTRUE) 
        {
            // 2. Tạo đối tượng cJSON
            cJSON *root = cJSON_CreateObject();
            cJSON_AddNumberToObject(root, "node",   power_data.node_id );
            cJSON_AddNumberToObject(root, "voltage", power_data.voltage );
            cJSON_AddNumberToObject(root, "current", power_data.current );
            cJSON_AddNumberToObject(root, "power",  power_data.power );
            cJSON_AddNumberToObject(root, "wh",      power_data.energy );

            // 3. Chuyển cJSON thành chuỗi (String)
            char *json_string = cJSON_PrintUnformatted(root);
            
            if (json_string != NULL) 
            {
                // 4. Tạo Topic động theo ID của Node (Ví dụ: sensor/node1/data)
                snprintf(topic, sizeof(topic), "sensor/node%d/data", power_data.node_id);

                // 5. Publish dữ liệu lên MQTT
                // QoS 1 để đảm bảo Broker nhận được dữ liệu đo điện quan trọng
                int msg_id = esp_mqtt_client_publish(client, topic, json_string, 0, 1, 0);
                
                if (msg_id != -1) 
                {
                    ESP_LOGI("MQTT_TASK", "Sent to %s: %s", topic, json_string);
                } 
                else 
                {
                    ESP_LOGE("MQTT_TASK", "Publish failed!");
                }

                // 6. GIẢI PHÓNG BỘ NHỚ (Cực kỳ quan trọng để tránh tràn RAM)
                free(json_string);
            }

            cJSON_Delete(root); // Giải phóng cây cJSON
        }
    }
}

void app_task_mqtt_send (void)
{
    xTaskCreate(task_mqtt_send, "MQTT_Send_Task", 4096, NULL, 5, NULL);
}
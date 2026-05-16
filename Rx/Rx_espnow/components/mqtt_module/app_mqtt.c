#include "app_mqtt.h"
#include "driver/gpio.h"
#include "app_espnow.h"
#include "struct_common.h"


#define TAG "MQTT_MODULE"
esp_mqtt_client_handle_t client = NULL;
uint32_t MQTT_CONNEECTED = 0;

// đặt tạm để test
void gpio_init(void)
{
    // gpio config
    gpio_config_t io ={
        .pin_bit_mask = (1ULL << LED_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io);
}


/*
    typedef struct {
    uint8_t node_id;
    uint8_t gain;
    uint8_t energy_cmd; 
    } __attribute__((packed)) node_ctrl_t;


    Cách hoạt động của hàm mqtt_json_parse_and_send:
    - Trong event mqtt data, khi nhận được Json hàm này sẽ được gọi để xử lý chuỗi Json rồi chuyển sang struct để
    truyền đi qua espnow cho các Node 
    - Tách Json thành các trường dữ liệu trong struct node_ctrl_t
    - Dựa vào node_id, tìm trong danh bạ ở id đó mac là bao nhiêu
    - Tiếp theo dùng hàm get_mac_by_node_id được lưu ở module espnow để copy mac của node
    - Sau khi có mac rồi thì gửi sturct ctrl_data qua esp_now_send cho node để cấu hình gain và mức Wh cần thiết
    - Cuối cùng giải phóng bộ nhớ cJSON đã cấp phát
*/

// Hàm xử lý chính
void mqtt_json_parse_and_send(const char *json_str) {
    cJSON *root = cJSON_Parse(json_str);
    if (root == NULL) 
    {
        ESP_LOGE("MQTT_JSON", "JSON không hợp lệ");
        return;
    }

    // 1. Chuyển JSON thành Struct
    node_ctrl_t ctrl_data;
    ctrl_data.node_id = cJSON_GetObjectItemCaseSensitive(root, "node")->valueint;
    ctrl_data.gain    = cJSON_GetObjectItemCaseSensitive(root, "gain")->valueint;
    ctrl_data.energy_cmd = cJSON_GetObjectItemCaseSensitive(root, "energy")->valueint;

    ESP_LOGI("MQTT_JSON", "Parsed: Node=%d, Gain=%d, Energy=%d", 
              ctrl_data.node_id, ctrl_data.gain, ctrl_data.energy_cmd);
    
    // Gửi vào Queue với ISR-safe version (vì MQTT handler có thể chạy từ ISR context)
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (xQueueSendFromISR(mqtt_to_espnow_queue, &ctrl_data, &xHigherPriorityTaskWoken) == pdTRUE) 
    {
        ESP_LOGI("MQTT_PRODUCER", "Dữ liệu Node %d đã vào Queue", ctrl_data.node_id);
        if (xHigherPriorityTaskWoken == pdTRUE) {
            portYIELD_FROM_ISR();
        }
    }
    
    cJSON_Delete(root); // Giải phóng bộ nhớ cJSON
}

void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        MQTT_CONNEECTED=1;
        
        msg_id = esp_mqtt_client_subscribe(client, "/topic/test1", 0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "/topic/test2", 1);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        MQTT_CONNEECTED=0;
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            // Tạo buffer tạm để chứa chuỗi JSON (vì mqtt data không có ký tự kết thúc \0)
        char *json_buf = strndup(event->data, event->data_len);
        /*
            Cấp phát bộ nhớ: Nó tự động tính toán và mượn một vùng nhớ (Heap) đủ để chứa dữ liệu.

            Sao chép: Nó copy đúng số lượng byte (event->data_len) từ gói tin MQTT sang vùng nhớ mới.

            Đóng gói: Nó tự động thêm ký tự \0 vào cuối chuỗi mới này.
        */

        // Gọi hàm xử lý
        mqtt_json_parse_and_send(json_buf);
        
        free(json_buf); // Giải phóng buffer tạm
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

void mqtt_app_start(void)
{
    ESP_LOGI(TAG, "STARTING MQTT");
    esp_mqtt_client_config_t mqttConfig = {
        .broker.address.uri = "mqtt://192.168.43.80:1883"};
    
    client = esp_mqtt_client_init(&mqttConfig);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);
}

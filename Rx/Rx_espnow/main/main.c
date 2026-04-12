#include <stdio.h>
#include "string.h"

#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_mac.h"
#include "esp_now.h"
#include "mqtt_client.h"
#include "cJSON.h"

#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_log.h"

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#define LED_PIN GPIO_NUM_2

#define WIFI_SSID      "realme 5s"
#define WIFI_PASS      "tona190804"

static const char *TAG = "MQTT_EXAMPLE";
esp_mqtt_client_handle_t client = NULL;
uint32_t MQTT_CONNEECTED = 0;

// declare functions
void send_cb(const esp_now_send_info_t *info, esp_now_send_status_t status);
void recv_cb(const esp_now_recv_info_t *info, const uint8_t *data, int len);
static void mqtt_app_start(void);


static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) 
    {
        esp_wifi_connect(); // WiFi bắt đầu thì ra lệnh kết nối
    } 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) 
    {
        printf("Mat ket noi WiFi, dang thu lai...\n");
        esp_wifi_connect(); // Bị văng mạng thì tự động kết nối lại
    } 
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) 
    {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        printf("DA KET NOI! IP: " IPSTR "\n", IP2STR(&event->ip_info.ip));
        
        // KHI CÓ IP RỒI MỚI NÊN LẤY CHANNEL VÀ LÀM VIỆC KHÁC

        uint8_t primary_chan;
        wifi_second_chan_t second_chan;
        esp_wifi_get_channel(&primary_chan, &second_chan);
        printf("Gateway dang truc o Channel: %d\n", primary_chan);
        mqtt_app_start();
    }
}

void wifi_init(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
        {
            nvs_flash_erase();
            ret = nvs_flash_init();
        }

    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();
    
    // Đăng ký event
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, NULL);
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();

}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
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
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);

        // 1. Phân tích chuỗi JSON (Dùng hàm ParseWithLength vì data MQTT không có \0)
        cJSON *root = cJSON_ParseWithLength(event->data, event->data_len);

        if (root == NULL) {
            ESP_LOGE(TAG, "JSON bi loi, khong the parse!");
            break;
        }

        // 2. Lay gia tri tu key "status"
        cJSON *status = cJSON_GetObjectItem(root, "status");
        cJSON *device = cJSON_GetObjectItem(root, "device");

        if (cJSON_IsNumber(status) && cJSON_IsString(device)) {
            if (strcmp(device->valuestring, "led") == 0) {
                int led_state = status->valueint; // Lay gia tri 0 hoac 1
                gpio_set_level(LED_PIN, led_state);
                ESP_LOGI(TAG, "Lenh JSON hop le: Device=%s, Status=%d", device->valuestring, led_state);
            }
        }

        // 3. QUAN TRONG: Phai giai phong bo nho sau khi dung xong
        cJSON_Delete(root);
    break;

    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

static void mqtt_app_start(void)
{
    ESP_LOGI(TAG, "STARTING MQTT");
    esp_mqtt_client_config_t mqttConfig = {
        .broker.address.uri = "mqtt://192.168.43.80:1883"};
    
    client = esp_mqtt_client_init(&mqttConfig);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);
}

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

void espnow_init(void)
{
    esp_now_init();
    esp_now_register_send_cb(send_cb);
    esp_now_register_recv_cb(recv_cb);
}

// MAC TX
uint8_t peer_mac[6] = {0xF0, 0x24, 0xF9, 0xEB, 0x6F, 0x6C};


// receive callback function
volatile uint8_t led_trigger = 0;
void recv_cb(const esp_now_recv_info_t *info, const uint8_t *data, int len)
{
    if (data[0] == 2)
    {
        led_trigger = 1;
    }
    
}

// send callback function
void send_cb(const esp_now_send_info_t *info, esp_now_send_status_t status)
{
    
}
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

void app_main(void)
{
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

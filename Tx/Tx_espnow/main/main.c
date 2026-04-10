#include <stdio.h>
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_log.h"
#include "string.h"
#include "nvs_flash.h"
#include "esp_now.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// declare functions
void send_cb(const esp_now_send_info_t *info, esp_now_send_status_t status);
void recv_cb(const esp_now_recv_info_t *info, const uint8_t *data, int len);
const char *TAG = "ESP_NOW_SENDER";
#define Input_pin 14
TaskHandle_t send_data_handle;

// Biến trạng thái để dò kênh
volatile bool is_gateway_found = false;
volatile esp_now_send_status_t last_status = ESP_NOW_SEND_FAIL;
uint8_t current_ch = 1;

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

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();

}

void espnow_init(void)
{
    esp_now_init();
    esp_now_register_send_cb(send_cb);
    esp_now_register_recv_cb(recv_cb);
}

// MAC RX
uint8_t peer_mac[6] = {0xFC, 0xB4, 0x67, 0x74, 0x61, 0x00};

// add peer
void add_peer(void)
{
    esp_now_peer_info_t peer = {0};
    memcpy(peer.peer_addr, peer_mac, 6);
    peer.channel = 0;
    peer.encrypt = false;

    esp_now_add_peer(&peer);
}



// send callback function
void send_cb(const esp_now_send_info_t *info, esp_now_send_status_t status)
{
    last_status = status;
    if (status == ESP_NOW_SEND_SUCCESS) {
        is_gateway_found = true;
        printf("Send OK\n");
    } else {
        printf("Send FAIL\n");
    }
}

// receive callback function
void recv_cb(const esp_now_recv_info_t *info, const uint8_t *data, int len)
{
    
}

// Task xử lý quét kênh và gửi dữ liệu
void send_data_task(void *pvParameters)
{
    uint8_t dummy_data = 0;
    while (1)
    {
        // Nếu chưa tìm thấy Gateway (hoặc bị mất dấu do đổi kênh)
        if (!is_gateway_found) {
            ESP_LOGI(TAG, "Dang do tim Gateway tai Channel: %d", current_ch);
            esp_wifi_set_channel(current_ch, WIFI_SECOND_CHAN_NONE);
            
            // Gửi thử một gói tin dummy để check kênh
            esp_now_send(peer_mac, &dummy_data, 1);
            
            vTaskDelay(pdMS_TO_TICKS(200)); // Đợi callback trả về kết quả
            
            if (is_gateway_found) {
                ESP_LOGI(TAG, ">>> DA TIM THAY GATEWAY TAI KENH %d <<<", current_ch);
            } else {
                current_ch = (current_ch % 11) + 1; // Quét từ 1-11
            }
        } 
        else {
            // TRẠNG THÁI ĐÃ KẾT NỐI: Chờ nhấn nút
            if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY) == 1) {
                uint8_t led_data = 2; // Lệnh bật LED
                esp_now_send(peer_mac, &led_data, sizeof(led_data));
                
                // Đợi 100ms để check trạng thái gửi
                vTaskDelay(pdMS_TO_TICKS(100));
                
                // Nếu gửi thất bại, có thể là do điện thoại đã đổi kênh
                if (last_status == ESP_NOW_SEND_FAIL) {
                    is_gateway_found = false; 
                    ESP_LOGW(TAG, "Mat ket noi, bat dau quet lai...");
                }
            }
        }
    }
}

// GPIO ISR handler
void IRAM_ATTR GPIO_ISR_handler (void *arg)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (send_data_handle != NULL)
    {
        vTaskNotifyGiveFromISR(send_data_handle, &xHigherPriorityTaskWoken);
    }

    if (xHigherPriorityTaskWoken)
    {
        portYIELD_FROM_ISR();
    }
}

// ISR GPIO config
void ISR_config (void)
{
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << Input_pin),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    gpio_config(&io);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(Input_pin, GPIO_ISR_handler, (void *) Input_pin);
}


void app_main(void)
{
    wifi_init();
    espnow_init();
    add_peer();
    ISR_config();
    xTaskCreatePinnedToCore(&send_data_task, "send_data_task", 2048, NULL, 5, &send_data_handle, 0);

}

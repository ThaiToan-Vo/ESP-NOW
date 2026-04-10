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
#include "esp_event.h"

#define LED_PIN GPIO_NUM_2

#define WIFI_SSID      "realme 5s"
#define WIFI_PASS      "tona190804"

// declare functions
void send_cb(const esp_now_send_info_t *info, esp_now_send_status_t status);
void recv_cb(const esp_now_recv_info_t *info, const uint8_t *data, int len);

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect(); // WiFi bắt đầu thì ra lệnh kết nối
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        printf("Mat ket noi WiFi, dang thu lai...\n");
        esp_wifi_connect(); // Bị văng mạng thì tự động kết nối lại
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        printf("DA KET NOI! IP: " IPSTR "\n", IP2STR(&event->ip_info.ip));
        
        // KHI CÓ IP RỒI MỚI NÊN LẤY CHANNEL VÀ LÀM VIỆC KHÁC
        uint8_t primary_chan;
        wifi_second_chan_t second_chan;
        esp_wifi_get_channel(&primary_chan, &second_chan);
        printf("Gateway dang truc o Channel: %d\n", primary_chan);
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

void app_main(void)
{
    wifi_init();
    espnow_init();

    // gpio config
    gpio_config_t io ={
        .pin_bit_mask = (1ULL << LED_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io);
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

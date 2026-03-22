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

#define Input_pin 14
TaskHandle_t send_data_handle;

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

    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE); // cùng channel
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
    peer.channel = 1;
    peer.encrypt = false;

    esp_now_add_peer(&peer);
}

// send data function 
void send_data(void)
{
    uint8_t data = 2;
    esp_now_send(peer_mac, &data, sizeof(data));
}

// send callback function
void send_cb(const esp_now_send_info_t *info, esp_now_send_status_t status)
{
    if (status == ESP_NOW_SEND_SUCCESS)
        printf("Send OK\n");
    else
        printf("Send FAIL\n");
}

// receive callback function
void recv_cb(const esp_now_recv_info_t *info, const uint8_t *data, int len)
{
    
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


// send data task

void send_data_task (void *pvParameters)
{
    while (1)
    {
        if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY) == 1 )
        {
            send_data();
        }
        
    }
}
void app_main(void)
{
    wifi_init();
    espnow_init();
    add_peer();
    ISR_config();
    xTaskCreatePinnedToCore(&send_data_task, "send_data_task", 2048, NULL, 5, &send_data_handle, 0);

}

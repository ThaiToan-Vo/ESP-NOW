#include <stdio.h>
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_log.h"
#include "string.h"
#include "nvs_flash.h"
#include "esp_now.h"


// declare functions
void send_cb(const esp_now_send_info_t *info, esp_now_send_status_t status);
void recv_cb(const esp_now_recv_info_t *info, const uint8_t *data, int len);


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
    char data[] = "hello";
    esp_now_send(peer_mac, (uint8_t *)data, sizeof(data));
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

void app_main(void)
{
    wifi_init();
    espnow_init();
    add_peer();

    while (1) {
        send_data();
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

#include "wifi_init.h"
#include "app_mqtt.h"


#define WIFI_SSID      "realme 5s"
#define WIFI_PASS      "tona190804"



void wifi_init(void)
{
    
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

void event_handler(void* arg, esp_event_base_t event_base,
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
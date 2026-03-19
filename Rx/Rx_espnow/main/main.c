#include <stdio.h>
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_log.h"
#include "string.h"
#include "nvs_flash.h"
#include "esp_now.h"
// receiver
void app_main(void)
{

    uint8_t mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, mac);

    printf("%02X:%02X:%02X:%02X:%02X:%02X\n",
       mac[0], mac[1], mac[2],
       mac[3], mac[4], mac[5]);
}

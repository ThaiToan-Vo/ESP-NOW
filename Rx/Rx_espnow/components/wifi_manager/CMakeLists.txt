idf_component_register(SRCS "wifi_init.c"
                    INCLUDE_DIRS "."
                    REQUIRES mqtt_module
                            json
                            esp_wifi 
                            esp_event
                            mqtt 
                            nvs_flash 
                            driver)
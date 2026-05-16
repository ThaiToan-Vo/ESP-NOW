idf_component_register(SRCS "app_mqtt.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_wifi
                            json 
                            mqtt 
                            esp_event
                            driver
                            espnow_module
                            common)
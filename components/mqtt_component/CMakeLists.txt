idf_component_register(SRCS "mqtt_component.c"
                    INCLUDE_DIRS "include"
                    REQUIRES nvs_flash esp_event esp_netif esp_wifi)

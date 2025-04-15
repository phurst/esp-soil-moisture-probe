idf_component_register(SRCS "wifi_component.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_wifi led_component
                    )

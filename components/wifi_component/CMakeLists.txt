idf_component_register(SRCS "wifi_component.c"
                    REQUIRES wifi_provisioning nvs_flash
                    INCLUDE_DIRS "include")

idf_component_register(SRCS "wifi_component.c" "salt_and_verifier.c" "qr_code.c"
                    REQUIRES wifi_provisioning nvs_flash espressif__qrcode
                    INCLUDE_DIRS "include")

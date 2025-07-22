idf_component_register(SRCS "sensor_component.c"
                       REQUIRES esp_driver_gpio esp_adc
                       INCLUDE_DIRS "." "include")

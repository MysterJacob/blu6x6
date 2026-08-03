idf_component_register(SRCS "ESP_CRSF.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_driver_uart esp_driver_gpio driver freertos)

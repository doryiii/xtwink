idf_component_register(SRCS "src/input.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_adc
                    PRIV_REQUIRES network esp_driver_gpio epd)

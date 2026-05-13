idf_component_register(SRCS "src/power.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_driver_gpio esp_adc adc_shared
                    PRIV_REQUIRES network)

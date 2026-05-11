idf_component_register(SRCS "src/power.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_driver_gpio
                    PRIV_REQUIRES network)

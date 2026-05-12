idf_component_register(SRCS "main.c" "app_display.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES network power input nvs_flash epd)

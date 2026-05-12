idf_component_register(SRCS "src/epd_driver.c" "src/drawing.c"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_driver_spi esp_driver_gpio)

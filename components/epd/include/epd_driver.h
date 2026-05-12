#pragma once

#include <stdint.h>
#include "driver/spi_master.h"

#define EPD_WIDTH  800
#define EPD_HEIGHT 480
#define FB_SIZE    (EPD_WIDTH * EPD_HEIGHT / 4) // 2 bits per pixel

void epd_init(spi_device_handle_t spi);
void epd_set_ram_area(spi_device_handle_t spi);
void epd_upload_red_ram(spi_device_handle_t spi, const uint8_t* fb_2bit);
void epd_write_grayscale(spi_device_handle_t spi, const uint8_t* fb_2bit, int refresh_mode);
void epd_deep_sleep(spi_device_handle_t spi);
void wait_busy();

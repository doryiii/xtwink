#include "app_display.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include <string.h>

#include "pins.h"
#include "epd_driver.h"
#include "drawing.h"

static const char *TAG = "APP_DISPLAY";

static QueueHandle_t display_cmd_queue = NULL;
static uint8_t framebuffer[FB_SIZE];
static spi_device_handle_t epd_spi;
static int current_image_idx = -1;

static const uint8_t* const* app_images = NULL;
static int app_image_count = 0;

void app_display_set_images(const uint8_t* const* images, int count) {
    app_images = images;
    app_image_count = count;
}

void app_display_send_cmd(display_cmd_t cmd) {
    if (display_cmd_queue) {
        xQueueSend(display_cmd_queue, &cmd, 0);
    }
}

static void display_task(void *pvParameters) {
    epd_init(epd_spi);
    current_image_idx = 0;
    
    // Draw first image on boot if available
    if (app_images && app_image_count > 0) {
        memcpy(framebuffer, app_images[current_image_idx], FB_SIZE);
    } else {
        memset(framebuffer, 0xFF, FB_SIZE); // White
    }
    epd_write_grayscale(epd_spi, framebuffer, 2); // Full refresh

    while (1) {
        display_cmd_t cmd;
        if (xQueueReceive(display_cmd_queue, &cmd, portMAX_DELAY)) {
            ESP_LOGI(TAG, "Received display command: %d", cmd);
            
            bool is_test_pattern = false;
            int refresh_mode = 1; // Default to partial refresh
            
            switch (cmd) {
                case DISPLAY_CMD_NEXT_IMAGE:
                    if (app_image_count > 0) {
                        current_image_idx = (current_image_idx + 1) % app_image_count;
                        memcpy(framebuffer, app_images[current_image_idx], FB_SIZE);
                    }
                    break;
                case DISPLAY_CMD_PREV_IMAGE:
                    if (app_image_count > 0) {
                        current_image_idx = (current_image_idx - 1 + app_image_count) % app_image_count;
                        memcpy(framebuffer, app_images[current_image_idx], FB_SIZE);
                    }
                    break;
                case DISPLAY_CMD_FULL_REFRESH:
                    refresh_mode = 2; // Full refresh
                    break;
                case DISPLAY_CMD_TEST_PATTERN:
                    is_test_pattern = true;
                    refresh_mode = 2; // Full refresh for test pattern
                    for (int y = 0; y < 480; y++) {
                        uint8_t fill_val = 0;
                        if (y < 120) fill_val = 0x00;
                        else if (y < 240) fill_val = 0x55;
                        else if (y < 360) fill_val = 0xAA;
                        else fill_val = 0xFF;
                        memset(framebuffer + (y * 200), fill_val, 200);
                    }
                    break;
            }

            if (!is_test_pattern && cmd != DISPLAY_CMD_FULL_REFRESH && app_image_count > 0) {
                char label[64];
                snprintf(label, sizeof(label), "Image %d/%d", current_image_idx + 1, app_image_count);
                draw_text(framebuffer, 10, 770, label, 0); 
            }
            
            epd_write_grayscale(epd_spi, framebuffer, refresh_mode);
            ESP_LOGI(TAG, "EPD update complete.");
        }
    }
}

void app_display_init(void) {
    ESP_LOGI(TAG, "Initializing display control...");
    
    gpio_reset_pin(PIN_NUM_DC);
    gpio_reset_pin(PIN_NUM_RST);
    gpio_reset_pin(PIN_NUM_BUSY);
    gpio_reset_pin(PIN_NUM_CS);
    gpio_set_direction(PIN_NUM_DC, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_NUM_RST, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_NUM_CS, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_NUM_BUSY, GPIO_MODE_INPUT);
    gpio_set_level(PIN_NUM_CS, 1);

    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_NUM_MISO,
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4092
    };
    
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 10 * 1000 * 1000, 
        .mode = 0,                               
        .spics_io_num = -1,              
        .queue_size = 7,                          
    };

    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &devcfg, &epd_spi));

    display_cmd_queue = xQueueCreate(5, sizeof(display_cmd_t));
    
    xTaskCreate(&display_task, "display_task", 8192, NULL, 5, NULL);
}
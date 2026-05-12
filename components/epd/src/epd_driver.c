#include "epd_driver.h"
#include "pins.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "EPD_DRIVER";

const unsigned char lut_grayscale[] = {
    // 00 black/white
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    // 01 light gray
    0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    // 10 gray
    0xAA, 0xA0, 0xA8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    // 11 dark gray
    0xAA, 0xA0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    // L4 (VCOM)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    // TP/RP groups (global timing)
    0x01, 0x01, 0x01, 0x01, 0x00,  // G0: A=1 B=1 C=1 D=1 RP=0 (4 frames)
    0x01, 0x01, 0x01, 0x01, 0x00,  // G1: A=1 B=1 C=1 D=1 RP=0 (4 frames)
    0x01, 0x01, 0x01, 0x01, 0x00,  // G2: A=0 B=0 C=0 D=0 RP=0 (4 frames)
    0x00, 0x00, 0x00, 0x00, 0x00,  // G3: A=0 B=0 C=0 D=0 RP=0
    0x00, 0x00, 0x00, 0x00, 0x00,  // G4: A=0 B=0 C=0 D=0 RP=0
    0x00, 0x00, 0x00, 0x00, 0x00,  // G5: A=0 B=0 C=0 D=0 RP=0
    0x00, 0x00, 0x00, 0x00, 0x00,  // G6: A=0 B=0 C=0 D=0 RP=0
    0x00, 0x00, 0x00, 0x00, 0x00,  // G7: A=0 B=0 C=0 D=0 RP=0
    0x00, 0x00, 0x00, 0x00, 0x00,  // G8: A=0 B=0 C=0 D=0 RP=0
    0x00, 0x00, 0x00, 0x00, 0x00,  // G9: A=0 B=0 C=0 D=0 RP=0

    // Frame rate
    0x8F, 0x8F, 0x8F, 0x8F, 0x8F,

    // Voltages (VGH, VSH1, VSH2, VSL, VCOM)
    0x17, 0x41, 0xA8, 0x32, 0x30,

    // Reserved
    0x00, 0x00
};

static uint8_t bw_chunk[4000];

static void send_cmd(spi_device_handle_t spi, uint8_t cmd) {
    gpio_set_level(PIN_NUM_CS, 0);
    gpio_set_level(PIN_NUM_DC, 0);
    spi_transaction_t t = {
        .length = 8, 
        .tx_buffer = &cmd 
    };
    spi_device_polling_transmit(spi, &t);
    gpio_set_level(PIN_NUM_CS, 1);
}

static void send_data(spi_device_handle_t spi, const uint8_t *data, int len) {
    if (len == 0) return;
    gpio_set_level(PIN_NUM_CS, 0);
    gpio_set_level(PIN_NUM_DC, 1);
    spi_transaction_t t = { 
        .length = (size_t)len * 8,
        .tx_buffer = data 
    };
    spi_device_polling_transmit(spi, &t);
    gpio_set_level(PIN_NUM_CS, 1);
}

static void send_data_byte(spi_device_handle_t spi, uint8_t data) {
    send_data(spi, &data, 1);
}

static void send_cmd_stream_start(spi_device_handle_t spi, uint8_t cmd) {
    gpio_set_level(PIN_NUM_CS, 0);
    gpio_set_level(PIN_NUM_DC, 0);
    spi_transaction_t t = { .length = 8, .tx_buffer = &cmd };
    spi_device_polling_transmit(spi, &t);
}

static void send_data_stream_chunk(spi_device_handle_t spi, const uint8_t *data, int len) {
    if (len == 0) return;
    gpio_set_level(PIN_NUM_DC, 1);
    spi_transaction_t t = { .length = (size_t)len * 8, .tx_buffer = data };
    spi_device_polling_transmit(spi, &t);
}

static void send_stream_end() {
    gpio_set_level(PIN_NUM_CS, 1);
}

void wait_busy() {
    while(gpio_get_level(PIN_NUM_BUSY) == 1) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void epd_set_ram_area(spi_device_handle_t spi) {
    send_cmd(spi, 0x11); send_data_byte(spi, 0x01);
    uint8_t xrange[] = { 0, 0, 799%256, 799/256 };
    send_cmd(spi, 0x44); send_data(spi, xrange, 4);
    uint8_t yrange[] = { 479%256, 479/256, 0, 0 };
    send_cmd(spi, 0x45); send_data(spi, yrange, 4);
    uint8_t xcnt[] = { 0, 0 };
    send_cmd(spi, 0x4E); send_data(spi, xcnt, 2);
    uint8_t ycnt[] = { 479%256, 479/256 };
    send_cmd(spi, 0x4F); send_data(spi, ycnt, 2);
}

void epd_init(spi_device_handle_t spi) {
    gpio_set_level(PIN_NUM_RST, 1); vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level(PIN_NUM_RST, 0); vTaskDelay(pdMS_TO_TICKS(2));
    gpio_set_level(PIN_NUM_RST, 1); vTaskDelay(pdMS_TO_TICKS(20));

    send_cmd(spi, 0x12);
    wait_busy();

    send_cmd(spi, 0x18); send_data_byte(spi, 0x80);
    uint8_t booster[] = {0xAE, 0xC7, 0xC3, 0xC0, 0x40};
    send_cmd(spi, 0x0C); send_data(spi, booster, 5);
    uint8_t doc[] = { (480-1)%256, (480-1)/256, 0x02 };
    send_cmd(spi, 0x01); send_data(spi, doc, 3);
    send_cmd(spi, 0x3C); send_data_byte(spi, 0x01);
    epd_set_ram_area(spi);
    send_cmd(spi, 0x46); send_data_byte(spi, 0xF7);
    wait_busy();
    send_cmd(spi, 0x47); send_data_byte(spi, 0xF7);
    wait_busy();
}

void epd_deep_sleep(spi_device_handle_t spi) {
    ESP_LOGI(TAG, "Properly powering down EPD analog rails...");
    send_cmd(spi, 0x21); send_data_byte(spi, 0x40); 
    send_cmd(spi, 0x22); send_data_byte(spi, 0x03); 
    send_cmd(spi, 0x20);
    wait_busy();
    send_cmd(spi, 0x10); send_data_byte(spi, 0x01); 
    ESP_LOGI(TAG, "EPD deep sleep command sent.");
}

void epd_upload_red_ram(spi_device_handle_t spi, const uint8_t* fb_2bit) {
    epd_set_ram_area(spi);
    send_cmd_stream_start(spi, 0x26);
    for (int i=0; i<FB_SIZE; i+=8000) {
        int chunk_len = (FB_SIZE - i) > 8000 ? 8000 : (FB_SIZE - i);
        int out_len = chunk_len / 2;
        for (int j = 0; j < out_len; j++) {
            uint8_t b0 = fb_2bit[i + j*2];
            uint8_t b1 = fb_2bit[i + j*2 + 1];
            uint8_t out = 0;
            for(int k=0; k<4; k++) {
                if (((b0 >> (6 - k*2)) & 3) == 3) out |= (1 << (7 - k));
            }
            for(int k=0; k<4; k++) {
                if (((b1 >> (6 - k*2)) & 3) == 3) out |= (1 << (3 - k));
            }
            bw_chunk[j] = out;
        }
        send_data_stream_chunk(spi, bw_chunk, out_len);
    }
    send_stream_end();
}

void epd_write_grayscale(spi_device_handle_t spi, const uint8_t* fb_2bit, int refresh_mode) {
    epd_set_ram_area(spi);
    send_cmd_stream_start(spi, 0x24);
    for (int i=0; i<FB_SIZE; i+=8000) {
        int chunk_len = (FB_SIZE - i) > 8000 ? 8000 : (FB_SIZE - i);
        int out_len = chunk_len / 2;
        for (int j = 0; j < out_len; j++) {
            uint8_t b0 = fb_2bit[i + j*2];
            uint8_t b1 = fb_2bit[i + j*2 + 1];
            uint8_t out = 0;
            for(int k=0; k<4; k++) {
                if (((b0 >> (6 - k*2)) & 3) == 3) out |= (1 << (7 - k));
            }
            for(int k=0; k<4; k++) {
                if (((b1 >> (6 - k*2)) & 3) == 3) out |= (1 << (3 - k));
            }
            bw_chunk[j] = out;
        }
        send_data_stream_chunk(spi, bw_chunk, out_len);
    }
    send_stream_end();
    
    if (refresh_mode == 0) {
        send_cmd(spi, 0x21); send_data_byte(spi, 0x00); send_data_byte(spi, 0x00);
        send_cmd(spi, 0x22); send_data_byte(spi, 0xFC); 
        send_cmd(spi, 0x20);
        wait_busy();
    } else if (refresh_mode == 1) {
        send_cmd(spi, 0x21); send_data_byte(spi, 0x40); send_data_byte(spi, 0x00);
        send_cmd(spi, 0x1A); send_data_byte(spi, 0x5A);
        send_cmd(spi, 0x22); send_data_byte(spi, 0xD7); 
        send_cmd(spi, 0x20);
        wait_busy();
    } else {
        send_cmd(spi, 0x21); send_data_byte(spi, 0x40); send_data_byte(spi, 0x00);
        send_cmd(spi, 0x18); send_data_byte(spi, 0x80);
        send_cmd(spi, 0x22); send_data_byte(spi, 0xF7); 
        send_cmd(spi, 0x20);
        wait_busy();
    }

    epd_set_ram_area(spi);
    send_cmd_stream_start(spi, 0x24);
    for (int i=0; i<FB_SIZE; i+=8000) {
        int chunk_len = (FB_SIZE - i) > 8000 ? 8000 : (FB_SIZE - i);
        int out_len = chunk_len / 2;
        for (int j = 0; j < out_len; j++) {
            uint8_t b0 = fb_2bit[i + j*2];
            uint8_t b1 = fb_2bit[i + j*2 + 1];
            uint8_t out = 0;
            for(int k=0; k<4; k++) {
                if (((b0 >> (6 - k*2)) & 3) == 1) out |= (1 << (7 - k));
            }
            for(int k=0; k<4; k++) {
                if (((b1 >> (6 - k*2)) & 3) == 1) out |= (1 << (3 - k));
            }
            bw_chunk[j] = out;
        }
        send_data_stream_chunk(spi, bw_chunk, out_len);
    }
    send_stream_end();

    epd_set_ram_area(spi);
    send_cmd_stream_start(spi, 0x26);
    for (int i=0; i<FB_SIZE; i+=8000) {
        int chunk_len = (FB_SIZE - i) > 8000 ? 8000 : (FB_SIZE - i);
        int out_len = chunk_len / 2;
        for (int j = 0; j < out_len; j++) {
            uint8_t b0 = fb_2bit[i + j*2];
            uint8_t b1 = fb_2bit[i + j*2 + 1];
            uint8_t out = 0;
            for(int k=0; k<4; k++) {
                uint8_t val = (b0 >> (6 - k*2)) & 3;
                if (val == 1 || val == 2) out |= (1 << (7 - k));
            }
            for(int k=0; k<4; k++) {
                uint8_t val = (b1 >> (6 - k*2)) & 3;
                if (val == 1 || val == 2) out |= (1 << (3 - k));
            }
            bw_chunk[j] = out;
        }
        send_data_stream_chunk(spi, bw_chunk, out_len);
    }
    send_stream_end();

    send_cmd(spi, 0x32);
    for(int i=0; i<105; i++) send_data_byte(spi, lut_grayscale[i]);
    send_cmd(spi, 0x03); send_data_byte(spi, lut_grayscale[105]); 
    send_cmd(spi, 0x04); send_data_byte(spi, lut_grayscale[106]); send_data_byte(spi, lut_grayscale[107]); send_data_byte(spi, lut_grayscale[108]); 
    send_cmd(spi, 0x2C); send_data_byte(spi, lut_grayscale[109]); 
    
    send_cmd(spi, 0x21); send_data_byte(spi, 0x00); send_data_byte(spi, 0x00);
    send_cmd(spi, 0x22); send_data_byte(spi, 0xC7); 
    send_cmd(spi, 0x20); 
    wait_busy();
}

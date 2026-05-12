#pragma once

#include <stdint.h>

typedef enum {
    EPD_CMD_NEXT_IMAGE,
    EPD_CMD_PREV_IMAGE,
    EPD_CMD_FULL_REFRESH,
    EPD_CMD_TEST_PATTERN
} epd_cmd_t;

void epd_control_init(void);
void epd_control_set_images(const uint8_t* const* images, int count);
void epd_send_cmd(epd_cmd_t cmd);

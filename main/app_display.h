#pragma once

#include <stdint.h>

typedef enum {
    DISPLAY_CMD_NEXT_IMAGE,
    DISPLAY_CMD_PREV_IMAGE,
    DISPLAY_CMD_FULL_REFRESH,
    DISPLAY_CMD_TEST_PATTERN
} display_cmd_t;

void app_display_init(void);
void app_display_set_images(const uint8_t* const* images, int count);
void app_display_send_cmd(display_cmd_t cmd);

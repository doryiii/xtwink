#pragma once

typedef enum {
    EPD_CMD_NEXT_IMAGE,
    EPD_CMD_PREV_IMAGE,
    EPD_CMD_FULL_REFRESH,
    EPD_CMD_TEST_PATTERN
} epd_cmd_t;

void epd_control_init(void);
void epd_send_cmd(epd_cmd_t cmd);

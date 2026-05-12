#pragma once

#include <stdint.h>

#define BTN_BACK 0
#define BTN_CONFIRM 1
#define BTN_LEFT 2
#define BTN_RIGHT 3
#define BTN_UP 4
#define BTN_DOWN 5
#define BTN_POWER 6

typedef void (*input_btn_cb_t)(int btn, int state);

void input_init(void);
void input_set_button_callback(input_btn_cb_t cb);

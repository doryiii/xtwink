#pragma once

#include <stdint.h>

void draw_pixel(uint8_t *fb, int lx, int ly, uint8_t color_val);
void draw_char(uint8_t *fb, int x, int y, char c, uint8_t color_val);
void draw_text(uint8_t *fb, int x, int y, const char *str, uint8_t color_val);

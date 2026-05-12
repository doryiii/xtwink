#include "drawing.h"
#include "font8x8_basic.h"

// Set a pixel in a 480x800 logical portrait framebuffer
// The physical panel is 800x480 landscape
// Values: 0=Black, 1=Dark Gray, 2=Light Gray, 3=White
void draw_pixel(uint8_t *fb, int lx, int ly, uint8_t color_val) {
    if (lx < 0 || lx >= 480 || ly < 0 || ly >= 800) return;
    int px = ly;
    int py = 479 - lx;
    
    int byte_idx = py * 200 + (px / 4);
    int bit_idx = 6 - ((px % 4) * 2); 
    
    fb[byte_idx] &= ~(3 << bit_idx);
    fb[byte_idx] |= (color_val << bit_idx);
}

// Draw a single character scaled by 2x for readability
void draw_char(uint8_t *fb, int x, int y, char c, uint8_t color_val) {
    if ((uint8_t)c > 127) return;
    uint8_t *bitmap = (uint8_t *)font8x8_basic[(int)c];
    for (int r = 0; r < 8; r++) {
        for (int c_idx = 0; c_idx < 8; c_idx++) {
            if (bitmap[r] & (1 << c_idx)) {
                for (int dy = 0; dy < 2; dy++) {
                    for (int dx = 0; dx < 2; dx++) {
                        draw_pixel(fb, x + c_idx*2 + dx, y + r*2 + dy, color_val);
                    }
                }
            }
        }
    }
}

void draw_text(uint8_t *fb, int x, int y, const char *str, uint8_t color_val) {
    while (*str) {
        draw_char(fb, x, y, *str, color_val);
        x += 16; // 8 pixels * 2x scale
        if (x >= 480) {
            x = 0;
            y += 16;
        }
        str++;
    }
}

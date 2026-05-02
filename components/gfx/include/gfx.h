#pragma once
#include <stdint.h>
#include "fb.h"

void gfx_clear   (fb_t *fb, uint16_t color);
void gfx_pixel   (fb_t *fb, int x, int y, uint16_t color);
void gfx_hline   (fb_t *fb, int x, int y, int len, uint16_t color);
void gfx_vline   (fb_t *fb, int x, int y, int len, uint16_t color);
void gfx_line    (fb_t *fb, int x0, int y0, int x1, int y1, uint16_t color);
void gfx_rect    (fb_t *fb, int x, int y, int w, int h, uint16_t color);
void gfx_text_5x7(fb_t *fb, int x, int y, const char *s, uint16_t color);

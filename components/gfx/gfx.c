#include "gfx.h"
#include <string.h>
#include <stdlib.h>

extern const uint8_t font5x7[96][5];  /* defined in font_5x7.c */

void gfx_clear(fb_t *fb, uint16_t color)
{
    for (int i = 0; i < fb->w * fb->h; ++i) fb->pixels[i] = color;
}

void gfx_pixel(fb_t *fb, int x, int y, uint16_t color)
{
    if ((unsigned)x >= (unsigned)fb->w || (unsigned)y >= (unsigned)fb->h) return;
    fb->pixels[y * fb->w + x] = color;
}

void gfx_hline(fb_t *fb, int x, int y, int len, uint16_t color)
{
    if ((unsigned)y >= (unsigned)fb->h) return;
    if (x < 0)            { len += x; x = 0; }
    if (x + len > fb->w)  { len = fb->w - x; }
    if (len <= 0) return;
    uint16_t *p = &fb->pixels[y * fb->w + x];
    for (int i = 0; i < len; ++i) p[i] = color;
}

void gfx_vline(fb_t *fb, int x, int y, int len, uint16_t color)
{
    if ((unsigned)x >= (unsigned)fb->w) return;
    if (y < 0)            { len += y; y = 0; }
    if (y + len > fb->h)  { len = fb->h - y; }
    if (len <= 0) return;
    uint16_t *p = &fb->pixels[y * fb->w + x];
    for (int i = 0; i < len; ++i) { *p = color; p += fb->w; }
}

void gfx_line(fb_t *fb, int x0, int y0, int x1, int y1, uint16_t color)
{
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    for (;;) {
        gfx_pixel(fb, x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void gfx_rect(fb_t *fb, int x, int y, int w, int h, uint16_t color)
{
    gfx_hline(fb, x,         y,         w, color);
    gfx_hline(fb, x,         y + h - 1, w, color);
    gfx_vline(fb, x,         y,         h, color);
    gfx_vline(fb, x + w - 1, y,         h, color);
}

void gfx_text_5x7(fb_t *fb, int x, int y, const char *s, uint16_t color)
{
    for (; *s; ++s) {
        unsigned char c = (unsigned char)*s;
        if (c < 32 || c > 127) c = '?';
        const uint8_t *glyph = font5x7[c - 32];
        /* If the glyph is all-zero AND it isn't ASCII space (32), the font
         * has no entry for this char — fall back to '?' so missing letters
         * are visible rather than silently blank. */
        if (c != ' ' &&
            glyph[0] == 0 && glyph[1] == 0 && glyph[2] == 0 &&
            glyph[3] == 0 && glyph[4] == 0) {
            glyph = font5x7['?' - 32];
        }
        for (int col = 0; col < 5; ++col) {
            uint8_t bits = glyph[col];
            for (int row = 0; row < 7; ++row) {
                if (bits & (1 << row)) gfx_pixel(fb, x + col, y + row, color);
            }
        }
        x += 6;
    }
}

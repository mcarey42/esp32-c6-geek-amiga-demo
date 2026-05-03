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

/* Per-pixel RGB565 alpha blend. alpha 0..255 (255 = opaque src). */
static inline uint16_t blend565(uint16_t dst, uint16_t src, uint8_t alpha)
{
    if (alpha >= 255) return src;
    if (alpha == 0)   return dst;
    uint16_t ia = 255 - alpha;
    uint8_t dr = (dst >> 11) & 0x1F;
    uint8_t dg = (dst >> 5)  & 0x3F;
    uint8_t db =  dst        & 0x1F;
    uint8_t sr = (src >> 11) & 0x1F;
    uint8_t sg = (src >> 5)  & 0x3F;
    uint8_t sb =  src        & 0x1F;
    uint8_t r = (uint8_t)((sr * alpha + dr * ia) >> 8);
    uint8_t g = (uint8_t)((sg * alpha + dg * ia) >> 8);
    uint8_t b = (uint8_t)((sb * alpha + db * ia) >> 8);
    return (uint16_t)((r << 11) | (g << 5) | b);
}

static void blend_hline(fb_t *fb, int xa, int xb, int y, uint16_t color, uint8_t alpha)
{
    if ((unsigned)y >= (unsigned)fb->h) return;
    if (xa > xb) { int t = xa; xa = xb; xb = t; }
    if (xa < 0) xa = 0;
    if (xb >= fb->w) xb = fb->w - 1;
    uint16_t *p = &fb->pixels[y * fb->w + xa];
    for (int x = xa; x <= xb; ++x, ++p) *p = blend565(*p, color, alpha);
}

void gfx_tri_filled_alpha(fb_t *fb,
                          int x0, int y0,
                          int x1, int y1,
                          int x2, int y2,
                          uint16_t color, uint8_t alpha)
{
    if (alpha == 0) return;
    /* Sort vertices ascending in y. */
    if (y0 > y1) { int t=y0;y0=y1;y1=t; t=x0;x0=x1;x1=t; }
    if (y0 > y2) { int t=y0;y0=y2;y2=t; t=x0;x0=x2;x2=t; }
    if (y1 > y2) { int t=y1;y1=y2;y2=t; t=x1;x1=x2;x2=t; }

    if (y2 == y0) {
        /* Degenerate flat tri. */
        int xmin = x0 < x1 ? x0 : x1; if (x2 < xmin) xmin = x2;
        int xmax = x0 > x1 ? x0 : x1; if (x2 > xmax) xmax = x2;
        blend_hline(fb, xmin, xmax, y0, color, alpha);
        return;
    }

    /* For each scanline y in [y0..y2], compute long-edge x and the
     * relevant short-edge x; fill between them with alpha blend. */
    for (int y = y0; y <= y2; ++y) {
        if (y < 0) continue;
        if (y >= fb->h) break;
        int xa = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
        int xb;
        if (y < y1) {
            if (y1 == y0) xb = x0;
            else xb = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
        } else {
            if (y2 == y1) xb = x1;
            else xb = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
        }
        blend_hline(fb, xa, xb, y, color, alpha);
    }
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

#include "scenes.h"
#include "fb.h"
#include "gfx.h"
#include <math.h>
#include <string.h>

#define POST_END_MS    2000
#define TITLE_BEGIN_MS 3000

static const char *POST_LINES[] = {
    "ESP32-C6 RISC-V @ 160 MHz",
    "MEMORY CHECK ... 512 KB OK",
    "ST7789 LCD 240x135 ... OK",
    "LOADING DEMO ...",
};
#define POST_LINE_COUNT ((int)(sizeof(POST_LINES) / sizeof(POST_LINES[0])))

static uint16_t copper_color(int row, uint32_t t)
{
    float h = (float)row / FB_H + (t / 8000.0f);
    h = h - (int)h;
    int seg = (int)(h * 6.0f);
    float f = h * 6.0f - seg;
    uint8_t v = 200;
    uint8_t p = 0;
    uint8_t q = (uint8_t)((1.0f - f) * 200);
    uint8_t r = (uint8_t)(f * 200);
    switch (seg % 6) {
        case 0: return fb_rgb565(v, r, p);
        case 1: return fb_rgb565(q, v, p);
        case 2: return fb_rgb565(p, v, r);
        case 3: return fb_rgb565(p, q, v);
        case 4: return fb_rgb565(r, p, v);
        default:return fb_rgb565(v, p, q);
    }
}

#define BAND_H 6

static void render(void *ctx, fb_t *fb, uint32_t t)
{
    (void)ctx;
    if (t < POST_END_MS) {
        gfx_clear(fb, fb_rgb565(0, 0, 0));
        int n_visible = (int)(t / 400) + 1;
        if (n_visible > POST_LINE_COUNT) n_visible = POST_LINE_COUNT;
        gfx_text_5x7(fb, 8, 4, "ESP-DEMOSYSTEM v1.0", fb_rgb565(80, 200, 80));
        for (int i = 0; i < n_visible; ++i) {
            gfx_text_5x7(fb, 8, 24 + i * 12, POST_LINES[i], fb_rgb565(200, 200, 200));
        }
        /* Blinking cursor at the bottom of the visible lines. */
        if ((t / 250) & 1) {
            int cy = 24 + n_visible * 12;
            gfx_text_5x7(fb, 8, cy, "_", fb_rgb565(200, 200, 200));
        }
        return;
    }

    if (t < TITLE_BEGIN_MS) {
        /* Snap-to-white flash transition. */
        uint8_t v = ((t - POST_END_MS) / 80) & 1 ? 255 : 0;
        gfx_clear(fb, fb_rgb565(v, v, v));
        return;
    }

    /* Title card with copper bar background. */
    enum { N_BANDS = (FB_H + BAND_H - 1) / BAND_H };
    uint16_t bands[N_BANDS];
    for (int b = 0; b < N_BANDS; ++b) bands[b] = copper_color(b, t);
    for (int y = 0; y < FB_H; ++y) gfx_hline(fb, 0, y, FB_W, bands[y / BAND_H]);

    const char *logo = "ESP32DEMO";
    int logo_w = (int)strlen(logo) * 6;
    int logo_x = (FB_W - logo_w) / 2;
    int logo_y = FB_H / 2 - 14;
    /* Chunky 3D logo: shadow, fill, highlight. */
    gfx_text_5x7(fb, logo_x + 2, logo_y + 2, logo, fb_rgb565(0, 0, 0));
    gfx_text_5x7(fb, logo_x + 1, logo_y + 1, logo, fb_rgb565(255, 0, 200));
    gfx_text_5x7(fb, logo_x,     logo_y,     logo, fb_rgb565(255, 255, 255));

    const char *sub = "PRESENTS - A TOUR THROUGH TIME";
    int sub_w = (int)strlen(sub) * 6;
    int sub_x = (FB_W - sub_w) / 2;
    int sub_y = logo_y + 16;
    gfx_text_5x7(fb, sub_x + 1, sub_y + 1, sub, fb_rgb565(0, 0, 0));
    gfx_text_5x7(fb, sub_x,     sub_y,     sub, fb_rgb565(255, 255, 255));
}

const scene_t SCENE_01_BOOT_TITLE = {
    .name = "boot_title",
    .est_duration_ms = 12000,
    .render = render,
};

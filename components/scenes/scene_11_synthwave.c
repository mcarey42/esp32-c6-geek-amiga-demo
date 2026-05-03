#include "scenes.h"
#include "fb.h"
#include "gfx.h"
#include <math.h>
#include <stdint.h>
#include <string.h>

#define SCENE_DURATION_MS 20000
#define HORIZON_Y         (FB_H * 11 / 20)  /* a touch above center */

static void render(void *ctx, fb_t *fb, uint32_t t)
{
    (void)ctx;

    /* Sky gradient: deep purple top -> magenta near horizon. */
    for (int y = 0; y < HORIZON_Y; ++y) {
        float k = (float)y / HORIZON_Y;
        uint8_t r = (uint8_t)(20 + k * 180);
        uint8_t g = (uint8_t)(0  + k * 30);
        uint8_t b = (uint8_t)(40 + k * 90);
        gfx_hline(fb, 0, y, FB_W, fb_rgb565(r, g, b));
    }

    /* Sun: gradient circle (semicircle visible above horizon). */
    int sun_cx = FB_W / 2;
    int sun_cy = HORIZON_Y - 8;
    int sun_r  = 28;
    for (int y = sun_cy - sun_r; y <= sun_cy + sun_r; ++y) {
        if (y < 0 || y >= HORIZON_Y) continue;
        int dy = y - sun_cy;
        int half_w = (int)sqrtf((float)(sun_r * sun_r - dy * dy));
        float k = (float)(y - (sun_cy - sun_r)) / (sun_r * 2);
        float gf = 220.0f - k * 200.0f;
        float bf = 80.0f  - k * 60.0f;
        if (gf < 0.0f) gf = 0.0f;
        if (gf > 220.0f) gf = 220.0f;
        if (bf < 0.0f) bf = 0.0f;
        uint8_t r = 255;
        uint8_t g = (uint8_t)gf;
        uint8_t b = (uint8_t)bf;
        /* Cut horizontal stripes for the classic outrun-sun look. */
        if (((y / 4) & 1) == 0)
            gfx_hline(fb, sun_cx - half_w, y, half_w * 2, fb_rgb565(r, g, b));
    }

    /* Ground: solid dark below horizon, then perspective grid. */
    for (int y = HORIZON_Y; y < FB_H; ++y) {
        gfx_hline(fb, 0, y, FB_W, fb_rgb565(10, 0, 20));
    }

    /* Horizontal grid lines: get closer as you near the horizon (perspective). */
    int n_horiz = 8;
    float scroll = (float)((t / 30) % 100) / 100.0f;
    for (int i = 0; i < n_horiz; ++i) {
        float r_norm = (i + scroll) / n_horiz;
        if (r_norm > 1.0f) r_norm -= 1.0f;
        /* Inverse mapping: row farther from horizon = higher r_norm. */
        int y = HORIZON_Y + (int)(r_norm * r_norm * (FB_H - HORIZON_Y));
        if (y < HORIZON_Y || y >= FB_H) continue;
        gfx_hline(fb, 0, y, FB_W, fb_rgb565(255, 80, 200));
    }

    /* Vertical grid lines: vanish from edges of horizon to bottom. */
    int n_vert = 11;
    int vp_x = FB_W / 2;
    for (int i = 0; i < n_vert; ++i) {
        float k = (float)i / (n_vert - 1) - 0.5f;   /* -0.5 .. 0.5 */
        int top_x = vp_x + (int)(k * 24);
        int bot_x = vp_x + (int)(k * FB_W * 1.4f);
        gfx_line(fb, top_x, HORIZON_Y, bot_x, FB_H - 1, fb_rgb565(255, 80, 200));
    }

    /* (Fade from white at the end — see below.) */
    /* "2026" chrome-ish text floating above the sun. */
    const char *year = "2026";
    int year_w = (int)strlen(year) * 6;
    int yx = (FB_W - year_w * 2) / 2;
    int yy = sun_cy - 24;
    /* Triple-render with offsets for chrome/3D effect. */
    gfx_text_5x7(fb, yx + 2, yy + 2, year, fb_rgb565(0, 0, 30));
    gfx_text_5x7(fb, yx + 1, yy + 1, year, fb_rgb565(0, 220, 255));
    gfx_text_5x7(fb, yx,     yy,     year, fb_rgb565(255, 255, 255));
    /* Same letters again offset to make them "wider/chunkier". */
    gfx_text_5x7(fb, yx + 6 + 2, yy + 2, year, fb_rgb565(0, 0, 30));
    gfx_text_5x7(fb, yx + 6 + 1, yy + 1, year, fb_rgb565(0, 220, 255));
    gfx_text_5x7(fb, yx + 6,     yy,     year, fb_rgb565(255, 255, 255));

    /* LIGHT_BURST diegetic transition: fade from white in the first 500ms
     * so Scene 10's exit-into-light visually delivers us here. */
    if (t < 500) {
        int k_q = (int)((float)(500 - t) / 500.0f * 32.0f);
        for (int i = 0; i < fb->w * fb->h; ++i) {
            uint16_t p = fb->pixels[i];
            uint8_t r = (p >> 11) & 0x1F;
            uint8_t g = (p >> 5)  & 0x3F;
            uint8_t b = (p)       & 0x1F;
            r = (uint8_t)(r + (31 - r) * k_q / 32);
            g = (uint8_t)(g + (63 - g) * k_q / 32);
            b = (uint8_t)(b + (31 - b) * k_q / 32);
            fb->pixels[i] = (uint16_t)((r << 11) | (g << 5) | b);
        }
    }
}

const scene_t SCENE_11_SYNTHWAVE = {
    .name = "synthwave",
    .est_duration_ms = SCENE_DURATION_MS,
    .render = render,
};

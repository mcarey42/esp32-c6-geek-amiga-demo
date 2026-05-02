#include "scenes.h"
#include "fb.h"
#include "gfx.h"
#include <math.h>
#include <string.h>

#define BAND_H        4
#define SCROLLER_Y    (FB_H - 16)
#define SCROLLER_AMP  6
#define SCROLLER_TEXT ">>> GREETZ TO SPACEBALLS - SCOOPEX - KEFRENS - SANITY - ANARCHY - FUTURE CREW - TBL <<< "

static uint16_t copper_color(int row, uint32_t t)
{
    /* Rotate hue across rows; cycle slowly with t. */
    float h = (float)row / FB_H + (t / 4000.0f);
    h = h - (int)h;
    /* HSV-ish with V=1, S=1 — cheap approximation. */
    int seg = (int)(h * 6.0f);
    float f = h * 6.0f - seg;
    uint8_t v = 255;
    uint8_t p = 0;
    uint8_t q = (uint8_t)((1.0f - f) * 255);
    uint8_t r = (uint8_t)(f * 255);
    switch (seg % 6) {
        case 0: return fb_rgb565(v, r, p);
        case 1: return fb_rgb565(q, v, p);
        case 2: return fb_rgb565(p, v, r);
        case 3: return fb_rgb565(p, q, v);
        case 4: return fb_rgb565(r, p, v);
        default:return fb_rgb565(v, p, q);
    }
}

static void render(void *ctx, fb_t *fb, uint32_t t)
{
    (void)ctx;
    /* Bands. Precompute one color per band, then fan out across rows. */
    enum { N_BANDS = (FB_H + BAND_H - 1) / BAND_H };
    uint16_t band_colors[N_BANDS];
    for (int b = 0; b < N_BANDS; ++b) band_colors[b] = copper_color(b, t);
    for (int y = 0; y < FB_H; ++y) {
        gfx_hline(fb, 0, y, FB_W, band_colors[y / BAND_H]);
    }
    /* Black strip behind scroller for legibility. */
    for (int y = SCROLLER_Y - 2; y < SCROLLER_Y + 9; ++y) {
        gfx_hline(fb, 0, y, FB_W, fb_rgb565(0, 0, 0));
    }
    /* Scroller text — shifted by t, sine-distorted vertically. */
    int n = (int)strlen(SCROLLER_TEXT);
    int pixels_per_char = 6;
    int total_w = n * pixels_per_char;
    int shift = (int)((t / 30) % total_w);
    for (int i = 0; i < n; ++i) {
        int cx = i * pixels_per_char - shift;
        /* Wrap the text horizontally so it scrolls infinitely. */
        cx = ((cx % total_w) + total_w) % total_w;
        cx -= 32;
        if (cx > FB_W) continue;
        if (cx < -pixels_per_char) continue;
        float phase = (cx + t * 0.05f) * 0.05f;
        int dy = (int)(SCROLLER_AMP * sinf(phase));
        char one[2] = { SCROLLER_TEXT[i], 0 };
        gfx_text_5x7(fb, cx, SCROLLER_Y + dy, one, fb_rgb565(255, 255, 255));
    }
}

const scene_t SCENE_02_COPPER_SCROLLER = {
    .name = "copper_scroller",
    .est_duration_ms = 30000,
    .render = render,
};

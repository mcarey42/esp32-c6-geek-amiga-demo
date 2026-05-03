#include "scenes.h"
#include "fb.h"
#include "gfx.h"
#include <math.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#define SCENE_DURATION_MS   25000
#define BREATHE_FADE_IN_MS   8000
#define BREATHE_PEAK_MS     12500
#define BREATHE_FADE_OUT_MS 17000

#define LUT_N 256

static float    s_sin_lut[LUT_N];
static uint16_t s_palette[256];
static uint8_t  s_radial[FB_H][FB_W];   /* pre-computed sqrt distance LUT */
static bool     s_lut_ready;

static void build_luts(void)
{
    for (int i = 0; i < LUT_N; ++i) {
        s_sin_lut[i] = sinf(2.0f * (float)M_PI * i / (float)LUT_N);
    }
    int cx = FB_W / 2, cy = FB_H / 2;
    /* Max possible distance = sqrt((W/2)^2 + (H/2)^2). Quantize to 0..255. */
    float max_d = sqrtf((float)(cx * cx + cy * cy));
    for (int y = 0; y < FB_H; ++y) {
        int dy = y - cy;
        for (int x = 0; x < FB_W; ++x) {
            int dx = x - cx;
            float d = sqrtf((float)(dx * dx + dy * dy));
            s_radial[y][x] = (uint8_t)(d / max_d * 255.0f);
        }
    }
    s_lut_ready = true;
}

static inline float lut_sin(float theta)
{
    int idx = (int)(theta * ((float)LUT_N / (2.0f * (float)M_PI))) & (LUT_N - 1);
    return s_sin_lut[idx];
}

static uint16_t blend_palette_color(uint8_t intensity, float walk)
{
    /* walk 0..1: 0 = angry red/orange, 1 = calming blue/violet */
    float r_targ = (1.0f - walk) * 1.00f + walk * 0.35f;
    float g_targ = (1.0f - walk) * 0.30f + walk * 0.05f;
    float b_targ = (1.0f - walk) * 0.05f + walk * 1.00f;
    uint8_t r = (uint8_t)(intensity * r_targ);
    uint8_t g = (uint8_t)(intensity * g_targ);
    uint8_t b = (uint8_t)(intensity * b_targ);
    return fb_rgb565(r, g, b);
}

static void build_palette(uint32_t t)
{
    float walk = (float)t / (float)SCENE_DURATION_MS;
    if (walk > 1.0f) walk = 1.0f;
    for (int i = 0; i < 256; ++i) {
        /* sine-modulated brightness across the 256 palette entries. */
        uint8_t intensity = (uint8_t)((s_sin_lut[i] * 0.5f + 0.5f) * 255.0f);
        s_palette[i] = blend_palette_color(intensity, walk);
    }
}

static void render(void *ctx, fb_t *fb, uint32_t t)
{
    (void)ctx;
    if (!s_lut_ready) build_luts();
    build_palette(t);

    float ts = t * 0.001f;
    /* Per-frame "phase" offsets so we evaluate sine-of-pixel-coord cheaply. */
    float t_a = ts * 1.0f;
    float t_b = ts * 0.7f;
    float t_c = ts * 0.5f;
    float t_d = ts * 0.3f;

    for (int y = 0; y < FB_H; ++y) {
        for (int x = 0; x < FB_W; ++x) {
            float v = lut_sin(x * 0.05f + t_a)
                    + lut_sin(y * 0.06f + t_b)
                    + lut_sin((x + y) * 0.03f + t_c)
                    + lut_sin(s_radial[y][x] * 0.04f + t_d);
            int idx = (int)((v + 4.0f) * (256.0f / 8.0f));
            if (idx < 0) idx = 0;
            if (idx > 255) idx = 255;
            fb->pixels[y * FB_W + x] = s_palette[idx];
        }
    }

    if (t >= BREATHE_FADE_IN_MS && t <= BREATHE_FADE_OUT_MS) {
        float k;
        if (t < BREATHE_PEAK_MS) {
            k = (float)(t - BREATHE_FADE_IN_MS) /
                (float)(BREATHE_PEAK_MS - BREATHE_FADE_IN_MS);
        } else {
            k = (float)(BREATHE_FADE_OUT_MS - t) /
                (float)(BREATHE_FADE_OUT_MS - BREATHE_PEAK_MS);
        }
        if (k < 0) k = 0;
        if (k > 1) k = 1;
        uint8_t bright = (uint8_t)(k * 255);
        uint16_t color = fb_rgb565(bright, bright, bright);
        const char *word = "breathe";
        int word_w = (int)strlen(word) * 6;
        int wx = (FB_W - word_w) / 2;
        int wy = FB_H / 2 - 4;
        /* Soft black drop-shadow for legibility against plasma. */
        if (bright > 30) gfx_text_5x7(fb, wx + 1, wy + 1, word,
                                       fb_rgb565(0, 0, 0));
        gfx_text_5x7(fb, wx, wy, word, color);
    }
}

const scene_t SCENE_05_PLASMA = {
    .name = "plasma",
    .est_duration_ms = SCENE_DURATION_MS,
    .render = render,
};

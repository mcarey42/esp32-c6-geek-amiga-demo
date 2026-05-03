#include "scenes.h"
#include "fb.h"
#include "gfx.h"
#include <math.h>
#include <stdint.h>
#include <stdbool.h>

#define TEX 64
#define TEX_MASK 63

#define SCENE_DURATION_MS 20000
#define TEX_HOLD_MS       6500   /* each texture displayed for ~6.5s */

static uint16_t s_tex[3][TEX][TEX];
static float    s_sin_lut[256];
static bool     s_ready;

static inline float lut_sin(float theta)
{
    int idx = (int)(theta * (256.0f / (2.0f * (float)M_PI))) & 0xFF;
    return s_sin_lut[idx];
}
static inline float lut_cos(float theta) { return lut_sin(theta + (float)M_PI * 0.5f); }

static void build_textures(void)
{
    /* tex 0: high-contrast 8x8 checker. */
    for (int y = 0; y < TEX; ++y) {
        for (int x = 0; x < TEX; ++x) {
            int c = ((x >> 3) ^ (y >> 3)) & 1;
            s_tex[0][y][x] = c ? fb_rgb565(255, 255, 255) : fb_rgb565(0, 0, 80);
        }
    }
    /* tex 1: XOR pattern, classic demoscene. */
    for (int y = 0; y < TEX; ++y) {
        for (int x = 0; x < TEX; ++x) {
            uint8_t v = (uint8_t)(x ^ y) << 1;
            s_tex[1][y][x] = fb_rgb565(v, v / 2, 255 - v);
        }
    }
    /* tex 2: concentric rings logo-ish. */
    for (int y = 0; y < TEX; ++y) {
        int dy = y - TEX / 2;
        for (int x = 0; x < TEX; ++x) {
            int dx = x - TEX / 2;
            int r = (int)sqrtf((float)(dx * dx + dy * dy));
            uint8_t band = (uint8_t)((r * 12) & 0xFF);
            s_tex[2][y][x] = fb_rgb565(255 - band, 80, band);
        }
    }
    for (int i = 0; i < 256; ++i)
        s_sin_lut[i] = sinf(2.0f * (float)M_PI * i / 256.0f);
    s_ready = true;
}

static void render(void *ctx, fb_t *fb, uint32_t t)
{
    (void)ctx;
    if (!s_ready) build_textures();

    int tex_idx = (int)(t / TEX_HOLD_MS) % 3;
    const uint16_t *tex = &s_tex[tex_idx][0][0];

    float ts = t * 0.001f;
    float angle = ts * 0.6f;
    /* Zoom oscillates between 0.5 (zoomed out) and 1.5 (zoomed in). */
    float zoom = 0.6f + 0.5f + 0.4f * lut_sin(ts * 0.4f);
    float c = lut_cos(angle) * zoom;
    float s = lut_sin(angle) * zoom;

    /* Half-res tiled render to keep within frame budget. */
#define RZ_W (FB_W / 2)
#define RZ_H (FB_H / 2)
    for (int py = 0; py < RZ_H; ++py) {
        int fy = py * 2;
        float yy = py - RZ_H * 0.5f;
        for (int px = 0; px < RZ_W; ++px) {
            int fx = px * 2;
            float xx = px - RZ_W * 0.5f;
            int u = (int)(xx * c - yy * s) & TEX_MASK;
            int v = (int)(xx * s + yy * c) & TEX_MASK;
            uint16_t color = tex[v * TEX + u];
            uint16_t *row0 = &fb->pixels[fy * FB_W + fx];
            row0[0] = color;
            if (fx + 1 < FB_W) row0[1] = color;
            if (fy + 1 < FB_H) {
                uint16_t *row1 = &fb->pixels[(fy + 1) * FB_W + fx];
                row1[0] = color;
                if (fx + 1 < FB_W) row1[1] = color;
            }
        }
    }
}

const scene_t SCENE_07_ROTOZOOMER = {
    .name = "rotozoomer",
    .est_duration_ms = SCENE_DURATION_MS,
    .render = render,
};

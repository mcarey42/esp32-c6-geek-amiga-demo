#include "scenes.h"
#include "fb.h"
#include "gfx.h"
#include <math.h>
#include <stdint.h>
#include <stdbool.h>

#define TEX 64
#define TEX_MASK 63
#define SCENE_DURATION_MS 25000
#define EXIT_BEGIN_MS     22000  /* last 3s = exit-into-light */

static uint8_t  s_tex_u[FB_H][FB_W];
static uint8_t  s_tex_v[FB_H][FB_W];
static uint16_t s_tex[TEX][TEX];
static bool     s_ready;

static void build_luts(void)
{
    int cx = FB_W / 2, cy = FB_H / 2;
    for (int y = 0; y < FB_H; ++y) {
        int dy = y - cy;
        for (int x = 0; x < FB_W; ++x) {
            int dx = x - cx;
            float d = sqrtf((float)(dx * dx + dy * dy));
            float a = atan2f((float)dy, (float)dx);
            /* u = scaled inverse depth, v = angle around tunnel. */
            float u = (d < 1.0f) ? 0.0f : (TEX * 32.0f / d);
            float v = (a / (2.0f * (float)M_PI) + 0.5f) * TEX;
            s_tex_u[y][x] = (uint8_t)((int)u & TEX_MASK);
            s_tex_v[y][x] = (uint8_t)((int)v & TEX_MASK);
        }
    }
    /* Tunnel wall texture: bright/dark checker with hue stripes. */
    for (int y = 0; y < TEX; ++y) {
        for (int x = 0; x < TEX; ++x) {
            int c = ((x >> 3) ^ (y >> 3)) & 1;
            uint8_t r = c ? 255 : 60;
            uint8_t g = (uint8_t)((y * 4) & 0xFF);
            uint8_t b = c ? 200 : 30;
            s_tex[y][x] = fb_rgb565(r, g, b);
        }
    }
    s_ready = true;
}

static void render(void *ctx, fb_t *fb, uint32_t t)
{
    (void)ctx;
    if (!s_ready) build_luts();

    /* Speed and twist build over time. */
    float speed = 0.04f + (t * 0.000004f);
    float twist = sinf(t * 0.0008f) * 8.0f;
    int scroll_u = (int)(t * speed * TEX) & TEX_MASK;
    int scroll_v = (int)(twist) & TEX_MASK;

    /* Brightness for the exit-into-light final phase. */
    float exit_k = 0.0f;
    if (t >= EXIT_BEGIN_MS) {
        exit_k = (float)(t - EXIT_BEGIN_MS) / (float)(SCENE_DURATION_MS - EXIT_BEGIN_MS);
        if (exit_k > 1.0f) exit_k = 1.0f;
    }

    for (int y = 0; y < FB_H; ++y) {
        for (int x = 0; x < FB_W; ++x) {
            int u = (s_tex_u[y][x] + scroll_u) & TEX_MASK;
            int v = (s_tex_v[y][x] + scroll_v) & TEX_MASK;
            uint16_t color = s_tex[v][u];
            if (exit_k > 0.0f) {
                /* Blend toward white based on depth (inverse u). Nearer
                 * pixels (small u from large d) brighten last. */
                uint8_t r = (uint8_t)(((color >> 11) & 0x1F) * (1.0f - exit_k) + 31 * exit_k);
                uint8_t g = (uint8_t)(((color >> 5)  & 0x3F) * (1.0f - exit_k) + 63 * exit_k);
                uint8_t b = (uint8_t)((color & 0x1F) * (1.0f - exit_k) + 31 * exit_k);
                color = (uint16_t)((r << 11) | (g << 5) | b);
            }
            fb->pixels[y * FB_W + x] = color;
        }
    }
}

const scene_t SCENE_10_TUNNEL = {
    .name = "tunnel",
    .est_duration_ms = SCENE_DURATION_MS,
    .render = render,
};

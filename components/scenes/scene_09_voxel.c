#include "scenes.h"
#include "fb.h"
#include "gfx.h"
#include <math.h>
#include <stdint.h>
#include <stdbool.h>

#define MAP_SZ 128
#define MAP_MASK (MAP_SZ - 1)

#define SCENE_DURATION_MS 30000

static uint8_t s_height[MAP_SZ][MAP_SZ];
static bool    s_map_ready;

static void build_map(void)
{
    /* Procedural rolling-dunes heightmap from low-frequency sines. */
    for (int y = 0; y < MAP_SZ; ++y) {
        for (int x = 0; x < MAP_SZ; ++x) {
            float h = sinf(x * 0.10f) * 18.0f
                    + cosf(y * 0.08f) * 22.0f
                    + sinf((x + y) * 0.05f) * 14.0f
                    + cosf((x - y) * 0.07f) * 10.0f;
            int v = 64 + (int)h;
            if (v < 0) v = 0;
            if (v > 255) v = 255;
            s_height[y][x] = (uint8_t)v;
        }
    }
    s_map_ready = true;
}

static uint16_t sky_color(int y_frac, float walk)
{
    /* y_frac 0 (top) .. 255 (horizon). walk 0 (noon) .. 1 (deep sunset). */
    /* Top: deep blue/violet at sunset, light blue at noon.
     * Horizon: warm orange at sunset, white-blue at noon. */
    float t = y_frac / 255.0f;
    float r_top = (1.0f - walk) * 0.40f + walk * 0.20f;
    float g_top = (1.0f - walk) * 0.55f + walk * 0.10f;
    float b_top = (1.0f - walk) * 0.95f + walk * 0.40f;

    float r_hor = (1.0f - walk) * 0.85f + walk * 1.00f;
    float g_hor = (1.0f - walk) * 0.85f + walk * 0.55f;
    float b_hor = (1.0f - walk) * 0.95f + walk * 0.30f;

    float r = r_top * (1.0f - t) + r_hor * t;
    float g = g_top * (1.0f - t) + g_hor * t;
    float b = b_top * (1.0f - t) + b_hor * t;
    return fb_rgb565((uint8_t)(r * 255), (uint8_t)(g * 255), (uint8_t)(b * 255));
}

static uint16_t terrain_color(uint8_t h, float walk)
{
    /* h is the height value at the sampled cell; lower = darker. */
    float lit = h / 255.0f;
    float r = (1.0f - walk) * (0.55f + 0.45f * lit) + walk * (0.40f + 0.55f * lit);
    float g = (1.0f - walk) * (0.50f + 0.40f * lit) + walk * (0.20f + 0.30f * lit);
    float b = (1.0f - walk) * (0.30f + 0.30f * lit) + walk * (0.05f + 0.15f * lit);
    if (r > 1) r = 1;
    if (g > 1) g = 1;
    if (b > 1) b = 1;
    return fb_rgb565((uint8_t)(r * 255), (uint8_t)(g * 255), (uint8_t)(b * 255));
}

static void render(void *ctx, fb_t *fb, uint32_t t)
{
    (void)ctx;
    if (!s_map_ready) build_map();

    float walk = (float)t / (float)SCENE_DURATION_MS;
    if (walk > 1) walk = 1;

    /* Sky: vertical gradient. Upper 60 rows. */
    int horizon_y = 60;
    for (int y = 0; y < horizon_y; ++y) {
        int yf = (int)((float)y / horizon_y * 255.0f);
        uint16_t c = sky_color(yf, walk);
        gfx_hline(fb, 0, y, FB_W, c);
    }

    /* Camera: drifting forward, slightly turning. */
    float ts = t * 0.001f;
    float cam_x = ts * 8.0f;
    float cam_y = ts * 6.0f;
    float cam_h = 80.0f;
    float yaw   = sinf(ts * 0.1f) * 0.3f;
    float dir_x = cosf(yaw);
    float dir_y = sinf(yaw);
    float right_x = -dir_y;
    float right_y =  dir_x;

    /* Comanche-style ray cast: for each x, march from near to far,
     * track tallest projected y, fill from that down to bottom. */
    int y_floor[FB_W];
    for (int x = 0; x < FB_W; ++x) y_floor[x] = FB_H;

    float fov_half = (float)FB_W * 0.5f;
    for (int z = 1; z < 80; ++z) {
        /* Compute the left and right edge of the view at this z. */
        float pl_x = cam_x + (dir_x * z) - (right_x * fov_half * z / 80.0f);
        float pl_y = cam_y + (dir_y * z) - (right_y * fov_half * z / 80.0f);
        float pr_x = cam_x + (dir_x * z) + (right_x * fov_half * z / 80.0f);
        float pr_y = cam_y + (dir_y * z) + (right_y * fov_half * z / 80.0f);
        float dx = (pr_x - pl_x) / FB_W;
        float dy = (pr_y - pl_y) / FB_W;
        float wx = pl_x;
        float wy = pl_y;
        for (int x = 0; x < FB_W; ++x) {
            int mx = ((int)wx) & MAP_MASK;
            int my = ((int)wy) & MAP_MASK;
            uint8_t h = s_height[my][mx];
            /* Project height to screen y. */
            float sy_f = horizon_y + (cam_h - h) * 50.0f / (float)z;
            int sy = (int)sy_f;
            if (sy < y_floor[x]) {
                if (sy < 0) sy = 0;
                if (sy < y_floor[x]) {
                    uint16_t c = terrain_color(h, walk);
                    for (int y = sy; y < y_floor[x] && y < FB_H; ++y)
                        fb->pixels[y * FB_W + x] = c;
                    y_floor[x] = sy;
                }
            }
            wx += dx;
            wy += dy;
        }
    }
}

const scene_t SCENE_09_VOXEL = {
    .name = "voxel",
    .est_duration_ms = SCENE_DURATION_MS,
    .render = render,
};

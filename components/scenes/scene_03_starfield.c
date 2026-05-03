#include "scenes.h"
#include "fb.h"
#include "gfx.h"
#include "gfx3d.h"
#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#define N_STARS         256
#define HYPERSPACE_MS   12000
#define FLASH_MS        17500
#define END_MS          18000

typedef struct { float x, y, z; } star_t;

static star_t  s_stars[N_STARS];
static bool    s_stars_ready;
static mat4_t  s_proj;
static bool    s_proj_ready;
static uint32_t s_lcg = 0xDEADBEEFu;

static float lcg_unit(void)
{
    s_lcg = s_lcg * 1103515245u + 12345u;
    return (float)((s_lcg >> 16) & 0x7FFF) / 32767.0f;
}

static void recycle_star(int i)
{
    s_stars[i].x = (lcg_unit() - 0.5f) * 40.0f;
    s_stars[i].y = (lcg_unit() - 0.5f) * 30.0f;
    s_stars[i].z = -lcg_unit() * 100.0f - 5.0f;
}

static void init_stars(void)
{
    s_lcg = 0xDEADBEEFu;
    for (int i = 0; i < N_STARS; ++i) recycle_star(i);
    s_stars_ready = true;
}

static void render(void *ctx, fb_t *fb, uint32_t t)
{
    (void)ctx;
    if (!s_stars_ready) init_stars();
    if (!s_proj_ready) {
        s_proj = mat4_perspective((float)M_PI / 3.0f,
                                  (float)FB_W / FB_H, 0.1f, 200.0f);
        s_proj_ready = true;
    }

    if (t >= FLASH_MS) {
        gfx_clear(fb, fb_rgb565(255, 255, 255));
        return;
    }

    gfx_clear(fb, fb_rgb565(0, 0, 8));

    float speed;
    if (t < HYPERSPACE_MS) {
        speed = 0.4f;
    } else {
        float ramp = (float)(t - HYPERSPACE_MS) / (float)(FLASH_MS - HYPERSPACE_MS);
        speed = 0.4f + ramp * ramp * 40.0f;
    }

    for (int i = 0; i < N_STARS; ++i) {
        float prev_z = s_stars[i].z;
        s_stars[i].z += speed;
        if (s_stars[i].z > -0.5f) {
            recycle_star(i);
            prev_z = s_stars[i].z;
        }
        int sx, sy;
        if (!gfx3d_project((vec3_t){s_stars[i].x, s_stars[i].y, s_stars[i].z},
                           s_proj, &sx, &sy)) continue;

        float depth_norm = (-s_stars[i].z) / 100.0f;
        if (depth_norm < 0) depth_norm = 0;
        if (depth_norm > 1) depth_norm = 1;
        uint8_t b = (uint8_t)((1.0f - depth_norm) * 255);
        uint16_t color = fb_rgb565(b, b, b);

        if (t < HYPERSPACE_MS) {
            gfx_pixel(fb, sx, sy, color);
            /* Slight glow for nearer stars. */
            if (b > 200) {
                gfx_pixel(fb, sx + 1, sy, color);
                gfx_pixel(fb, sx, sy + 1, color);
            }
        } else {
            int psx, psy;
            if (gfx3d_project((vec3_t){s_stars[i].x, s_stars[i].y, prev_z},
                              s_proj, &psx, &psy)) {
                gfx_line(fb, psx, psy, sx, sy, color);
            } else {
                gfx_pixel(fb, sx, sy, color);
            }
        }
    }
}

const scene_t SCENE_03_STARFIELD = {
    .name = "starfield",
    .est_duration_ms = END_MS,
    .render = render,
};

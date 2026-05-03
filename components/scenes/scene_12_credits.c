#include "scenes.h"
#include "fb.h"
#include "gfx.h"
#include "gfx3d.h"
#include <math.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#define SCENE_DURATION_MS 25000

static const char *CREDITS[] = {
    "ESP32DEMO",
    "",
    "A TOUR THROUGH TIME",
    "",
    "CODE & DESIGN",
    "  YOU + CLAUDE",
    "",
    "HARDWARE",
    "  ESP32-C6-GEEK",
    "",
    "INSPIRED BY",
    "  SPACEBALLS",
    "  KEFRENS",
    "  SCOOPEX",
    "  SANITY",
    "  FUTURE CREW",
    "  TBL",
    "",
    "GREETZ TO ALL THE",
    "DEMOSCENE LEGENDS",
    "",
    "2026",
    "",
    "",
    "...AND THE LOOP",
    "BEGINS AGAIN",
};
#define CREDITS_LEN ((int)(sizeof(CREDITS) / sizeof(CREDITS[0])))
#define LINE_H 12

/* Tiny X-Wing-ish wireframe — placeholder for the cameo callback. */
static const vec3_t s_xwing_verts[] = {
    {-2.0f, 0.0f, 0.0f}, {2.0f, 0.0f, 0.0f},   /* fuselage line */
    {-2.0f, 0.5f, 0.5f}, {-2.0f, -0.5f, -0.5f},
    {-2.0f, 0.5f, -0.5f}, {-2.0f, -0.5f, 0.5f},
};
static const uint16_t s_xwing_edges[] = {
    0, 1,        /* fuselage */
    0, 2,  0, 3, /* upper wings */
    0, 4,  0, 5, /* lower wings */
};
static const model_t s_xwing = {
    .verts = s_xwing_verts, .edges = s_xwing_edges,
    .n_edges = sizeof(s_xwing_edges) / (sizeof(uint16_t) * 2),
};

static mat4_t s_proj;
static bool   s_proj_ready;

static uint16_t copper_band(int row, uint32_t t)
{
    int phase = (row + (int)(t / 30)) & 0xFF;
    int seg = (phase / 43) % 6;
    uint8_t v = 200;
    switch (seg) {
        case 0: return fb_rgb565(v, 0, 0);
        case 1: return fb_rgb565(v, v / 2, 0);
        case 2: return fb_rgb565(0, v, 0);
        case 3: return fb_rgb565(0, v, v);
        case 4: return fb_rgb565(0, 0, v);
        default:return fb_rgb565(v, 0, v);
    }
}

static void render(void *ctx, fb_t *fb, uint32_t t)
{
    (void)ctx;
    if (!s_proj_ready) {
        s_proj = mat4_perspective((float)M_PI / 3.0f,
                                  (float)FB_W / FB_H, 0.1f, 100.0f);
        s_proj_ready = true;
    }
    /* Black background. */
    gfx_clear(fb, fb_rgb565(0, 0, 0));

    /* X-Wing cameo: drifts across the background slowly. */
    float ts = t * 0.001f;
    float cam_x = -8.0f + (ts * 1.2f);  /* sweeps left to right over ~13s */
    if (cam_x < 12.0f) {
        mat4_t world = mat4_translation(cam_x, 1.5f, -8.0f);
        world = mat4_mul(world, mat4_rotation_y(ts * 0.6f));
        gfx3d_draw_wireframe(fb, &s_xwing, world, s_proj, fb_rgb565(60, 60, 90));
    }

    /* Vertical scroller — credits move up over the scene. */
    /* Speed: total scroll distance = (CREDITS_LEN+1)*LINE_H + FB_H, divided
     * by SCENE_DURATION_MS to fill the scene. */
    float pix_per_ms = (float)(CREDITS_LEN * LINE_H + FB_H) /
                       (float)SCENE_DURATION_MS;
    int y0 = FB_H - (int)(t * pix_per_ms);
    for (int i = 0; i < CREDITS_LEN; ++i) {
        int y = y0 + i * LINE_H;
        if (y < -8 || y >= FB_H) continue;
        const char *line = CREDITS[i];
        if (!line[0]) continue;
        int line_w = (int)strlen(line) * 6;
        int x = (FB_W - line_w) / 2;
        /* Drop shadow then text. */
        gfx_text_5x7(fb, x + 1, y + 1, line, fb_rgb565(0, 0, 0));
        gfx_text_5x7(fb, x,     y,     line, fb_rgb565(255, 255, 255));
    }

    /* Copper ribbon at the bottom. */
    for (int y = FB_H - 6; y < FB_H; ++y) {
        gfx_hline(fb, 0, y, FB_W, copper_band(y - (FB_H - 6), t));
    }
}

const scene_t SCENE_12_CREDITS = {
    .name = "credits",
    .est_duration_ms = SCENE_DURATION_MS,
    .render = render,
};

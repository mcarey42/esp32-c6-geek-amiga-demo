/* Scene 06 — Metaballs CRYSTALLIZE into Glenz Vectors.
 *
 * Phases:
 *   0..12000   ms : pure metaballs (4 blobs in interfering circles)
 *   12000..14000 : crystallize transition — blobs shrink while a glenz
 *                  octahedron grows in (alpha-blended over the dimming
 *                  blob field). Per-pixel "noise dissolve" feel by way
 *                  of decaying blob isosurface threshold.
 *   14000..25000 : pure glenz octahedron rotating in 3D, faces drawn
 *                  as translucent shaded triangles.
 *
 * Same MB_W/MB_H half-res tile trick as Scene 05 for the metaball pass.
 */

#include "scenes.h"
#include "fb.h"
#include "gfx.h"
#include "gfx3d.h"
#include <math.h>
#include <stdint.h>
#include <stdbool.h>

#define MB_W (FB_W / 2)
#define MB_H (FB_H / 2)
#define N_BLOBS 4

#define SCENE_DURATION_MS 25000
#define BLOB_END_MS       12000
#define CRYSTAL_END_MS    14000

static float    s_sin_lut[256];
static bool     s_sin_ready;
static mat4_t   s_proj;
static bool     s_proj_ready;

static void build_sin_lut(void)
{
    for (int i = 0; i < 256; ++i)
        s_sin_lut[i] = sinf(2.0f * (float)M_PI * i / 256.0f);
    s_sin_ready = true;
}

static inline float lut_sin(float theta)
{
    int idx = (int)(theta * (256.0f / (2.0f * (float)M_PI))) & 0xFF;
    return s_sin_lut[idx];
}
static inline float lut_cos(float theta) { return lut_sin(theta + (float)M_PI * 0.5f); }

/* ---- Octahedron face data for glenz rendering ----
 * 6 vertices, 8 triangular faces. */
static const vec3_t s_octa_verts[6] = {
    { 1, 0, 0}, {-1, 0, 0},
    { 0, 1, 0}, { 0,-1, 0},
    { 0, 0, 1}, { 0, 0,-1},
};
static const uint8_t s_octa_faces[8][3] = {
    {0, 2, 4}, {0, 4, 3}, {0, 3, 5}, {0, 5, 2},
    {1, 4, 2}, {1, 3, 4}, {1, 5, 3}, {1, 2, 5},
};
/* Per-face base hue indices (so each face gets a distinct color). */
static const uint16_t s_octa_face_color[8] = {
    /* warm reds, then cool blues — palette matches the metaball walk */
    /* Picked to look "crystal-y" with alpha blend over a dark bg. */
    /* These are RGB565 values. */
    0xF8C0,   /* warm pink-orange */
    0xFC60,   /* amber */
    0xCFE0,   /* yellow-green */
    0x07F8,   /* aqua */
    0x041F,   /* deep blue */
    0x781F,   /* violet */
    0xC81F,   /* magenta */
    0xF810,   /* salmon */
};

/* Render the metaball field (full at intensity_scale=1.0, faded at 0). */
static void render_blobs(fb_t *fb, uint32_t t, float intensity_scale)
{
    float ts = t * 0.001f;
    float bx[N_BLOBS], by[N_BLOBS], br[N_BLOBS];
    for (int i = 0; i < N_BLOBS; ++i) {
        float a = ts * (0.5f + i * 0.13f) + i * 1.7f;
        bx[i] = MB_W * 0.5f + lut_cos(a) * (MB_W * 0.30f);
        by[i] = MB_H * 0.5f + lut_sin(a * 1.3f + i * 0.7f) * (MB_H * 0.30f);
        /* Radius shrinks during crystallize phase via intensity_scale. */
        br[i] = ((MB_W * 0.18f) + 4.0f * lut_sin(ts * 1.4f + i)) * intensity_scale;
    }
    float walk = (float)t / (float)SCENE_DURATION_MS;
    if (walk > 1.0f) walk = 1.0f;
    float r_targ = (1.0f - walk) * 1.0f + walk * 0.4f;
    float g_targ = (1.0f - walk) * 0.5f + walk * 0.1f;
    float b_targ = (1.0f - walk) * 0.0f + walk * 0.9f;

    for (int py = 0; py < MB_H; ++py) {
        int fy = py * 2;
        for (int px = 0; px < MB_W; ++px) {
            int fx = px * 2;
            float sum = 0.0f;
            for (int i = 0; i < N_BLOBS; ++i) {
                float dx = px - bx[i];
                float dy = py - by[i];
                float d2 = dx * dx + dy * dy + 1.0f;
                sum += (br[i] * br[i]) / d2;
            }
            uint8_t intensity;
            if (sum > 1.6f) intensity = 255;
            else if (sum > 0.8f) {
                float k = (sum - 0.8f) / 0.8f;
                intensity = (uint8_t)(60.0f + k * 195.0f);
            } else {
                float k = sum / 0.8f;
                intensity = (uint8_t)(k * 60.0f);
            }
            uint8_t r = (uint8_t)(intensity * r_targ);
            uint8_t g = (uint8_t)(intensity * g_targ);
            uint8_t b = (uint8_t)(intensity * b_targ);
            uint16_t color = fb_rgb565(r, g, b);
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

static void render_glenz(fb_t *fb, uint32_t t, uint8_t alpha)
{
    if (!s_proj_ready) {
        s_proj = mat4_perspective((float)M_PI / 3.0f,
                                  (float)FB_W / FB_H, 0.1f, 100.0f);
        s_proj_ready = true;
    }
    float ts = t * 0.001f;
    mat4_t world = mat4_translation(0, 0, -3.5f);
    world = mat4_mul(world, mat4_rotation_y(ts * 0.6f));
    world = mat4_mul(world, mat4_rotation_x(ts * 0.4f));

    /* Project all 6 vertices once. */
    int sx[6], sy[6];
    int ok[6];
    for (int v = 0; v < 6; ++v) {
        vec3_t vw = mat4_transform_point(world, s_octa_verts[v]);
        ok[v] = gfx3d_project(vw, s_proj, &sx[v], &sy[v]);
    }
    /* Draw each face as an alpha-blended triangle. No back-face cull
     * (transparency is supposed to show all faces); painter ordering
     * by face midpoint depth roughly approximates back-to-front. */
    /* Compute face depth (avg z of its 3 verts in view space). */
    float fz[8];
    int order[8];
    for (int f = 0; f < 8; ++f) {
        const uint8_t *idx = s_octa_faces[f];
        vec3_t v0 = mat4_transform_point(world, s_octa_verts[idx[0]]);
        vec3_t v1 = mat4_transform_point(world, s_octa_verts[idx[1]]);
        vec3_t v2 = mat4_transform_point(world, s_octa_verts[idx[2]]);
        fz[f] = (v0.z + v1.z + v2.z) / 3.0f;   /* more negative = farther */
        order[f] = f;
    }
    /* Insertion sort: most negative z first (back to front). */
    for (int i = 1; i < 8; ++i) {
        int o = order[i];
        float z = fz[o];
        int j = i - 1;
        while (j >= 0 && fz[order[j]] > z) {
            order[j + 1] = order[j];
            --j;
        }
        order[j + 1] = o;
    }
    /* Draw faces in back-to-front order. */
    for (int k = 0; k < 8; ++k) {
        int f = order[k];
        const uint8_t *idx = s_octa_faces[f];
        if (!ok[idx[0]] || !ok[idx[1]] || !ok[idx[2]]) continue;
        gfx_tri_filled_alpha(fb,
                             sx[idx[0]], sy[idx[0]],
                             sx[idx[1]], sy[idx[1]],
                             sx[idx[2]], sy[idx[2]],
                             s_octa_face_color[f], alpha);
    }
    /* Wireframe overlay so the crystal edges read clearly. */
    for (int f = 0; f < 8; ++f) {
        const uint8_t *idx = s_octa_faces[f];
        if (!ok[idx[0]] || !ok[idx[1]] || !ok[idx[2]]) continue;
        gfx_line(fb, sx[idx[0]], sy[idx[0]], sx[idx[1]], sy[idx[1]], fb_rgb565(255, 255, 255));
        gfx_line(fb, sx[idx[1]], sy[idx[1]], sx[idx[2]], sy[idx[2]], fb_rgb565(255, 255, 255));
        gfx_line(fb, sx[idx[2]], sy[idx[2]], sx[idx[0]], sy[idx[0]], fb_rgb565(255, 255, 255));
    }
}

static void render(void *ctx, fb_t *fb, uint32_t t)
{
    (void)ctx;
    if (!s_sin_ready) build_sin_lut();

    if (t < BLOB_END_MS) {
        render_blobs(fb, t, 1.0f);
    } else if (t < CRYSTAL_END_MS) {
        /* Crystallize: blobs shrink (intensity 1 -> 0), glenz alpha
         * grows (0 -> ~140). Both layered, with the blobs underneath. */
        float k = (float)(t - BLOB_END_MS) / (float)(CRYSTAL_END_MS - BLOB_END_MS);
        render_blobs(fb, t, 1.0f - k);
        uint8_t alpha = (uint8_t)(k * 140.0f);
        render_glenz(fb, t, alpha);
    } else {
        /* Pure glenz over a dim background (already cleared by previous
         * blob residue having faded out — but be safe and clear here). */
        gfx_clear(fb, fb_rgb565(8, 0, 24));
        render_glenz(fb, t, 140);
    }
}

const scene_t SCENE_06_METABALLS = {
    .name = "metaballs_glenz",
    .est_duration_ms = SCENE_DURATION_MS,
    .render = render,
};

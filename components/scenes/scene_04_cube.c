/* Scene 04 — Vector Parade. The symbol is still SCENE_04_CUBE for back-compat
 * with the timeline + tests, but the scene now plays a 7-stage tour through
 * wireframe geometry, ending in a Star Wars dogfight + Death Star reveal.
 *
 * Stages (timestamps in ms within the scene):
 *   0      cube           (5s)
 *   5000   cylinder       (5s)
 *   10000  octahedron     (4s)
 *   14000  icosahedron    (4s)
 *   18000  dodecahedron   (5s)
 *   23000  X-Wing solo    (7s, S-foils split mid-stage)
 *   30000  X-Wing + TIE   (7s, exchange "vector shots")
 *   37000  pullback + Death Star reveal (8s, end at 45s) */

#include "scenes.h"
#include "gfx.h"
#include "gfx3d.h"
#include <math.h>
#include <stdbool.h>
#include <stddef.h>

#define SCENE_DURATION_MS 45000

/* ---- Cube (8 verts, 12 edges) ---- */
static const vec3_t s_cube_verts[8] = {
    {-1,-1,-1}, { 1,-1,-1}, { 1, 1,-1}, {-1, 1,-1},
    {-1,-1, 1}, { 1,-1, 1}, { 1, 1, 1}, {-1, 1, 1},
};
static const uint16_t s_cube_edges[] = {
    0,1, 1,2, 2,3, 3,0,
    4,5, 5,6, 6,7, 7,4,
    0,4, 1,5, 2,6, 3,7,
};
static const model_t s_cube = {
    .verts = s_cube_verts, .edges = s_cube_edges,
    .n_edges = sizeof(s_cube_edges) / (sizeof(uint16_t) * 2),
};

/* ---- Cylinder (24 verts, 36 edges) — lazy-built ---- */
#define CYL_SEG 12
static vec3_t   s_cyl_verts[CYL_SEG * 2];
static uint16_t s_cyl_edges[(CYL_SEG * 3) * 2];
static model_t  s_cyl;
static bool     s_cyl_ready;

static void build_cylinder(void)
{
    for (int i = 0; i < CYL_SEG; ++i) {
        float a = (float)i * 2.0f * (float)M_PI / CYL_SEG;
        float c = cosf(a), s = sinf(a);
        s_cyl_verts[i].x          = c;       s_cyl_verts[i].y          = -1; s_cyl_verts[i].z          = s;
        s_cyl_verts[i + CYL_SEG].x = c;      s_cyl_verts[i + CYL_SEG].y =  1; s_cyl_verts[i + CYL_SEG].z = s;
    }
    int e = 0;
    for (int i = 0; i < CYL_SEG; ++i) {
        s_cyl_edges[e++] = (uint16_t)i;
        s_cyl_edges[e++] = (uint16_t)((i + 1) % CYL_SEG);
        s_cyl_edges[e++] = (uint16_t)(i + CYL_SEG);
        s_cyl_edges[e++] = (uint16_t)(((i + 1) % CYL_SEG) + CYL_SEG);
        s_cyl_edges[e++] = (uint16_t)i;
        s_cyl_edges[e++] = (uint16_t)(i + CYL_SEG);
    }
    s_cyl.verts = s_cyl_verts;
    s_cyl.edges = s_cyl_edges;
    s_cyl.n_edges = e / 2;
    s_cyl_ready = true;
}

/* ---- Octahedron (6 verts, 12 edges) ---- */
static const vec3_t s_octa_verts[6] = {
    { 1, 0, 0}, {-1, 0, 0},
    { 0, 1, 0}, { 0,-1, 0},
    { 0, 0, 1}, { 0, 0,-1},
};
static const uint16_t s_octa_edges[] = {
    0,2, 0,3, 0,4, 0,5,
    1,2, 1,3, 1,4, 1,5,
    2,4, 4,3, 3,5, 5,2,
};
static const model_t s_octa = {
    .verts = s_octa_verts, .edges = s_octa_edges,
    .n_edges = sizeof(s_octa_edges) / (sizeof(uint16_t) * 2),
};

/* ---- Icosahedron (12 verts, 30 edges) ---- */
#define PHI 1.6180339887f
static const vec3_t s_ico_verts[12] = {
    { 0,  1,  PHI}, { 0,  1, -PHI}, { 0, -1,  PHI}, { 0, -1, -PHI},
    { 1,  PHI, 0}, { 1, -PHI, 0}, {-1,  PHI, 0}, {-1, -PHI, 0},
    { PHI, 0,  1}, { PHI, 0, -1}, {-PHI, 0,  1}, {-PHI, 0, -1},
};
static const uint16_t s_ico_edges[] = {
    0,2, 0,4, 0,6, 0,8, 0,10,
    1,3, 1,4, 1,6, 1,9, 1,11,
    2,5, 2,7, 2,8, 2,10,
    3,5, 3,7, 3,9, 3,11,
    4,6, 4,8, 4,9,
    5,7, 5,8, 5,9,
    6,10, 6,11,
    7,10, 7,11,
    8,9,
    10,11,
};
static const model_t s_ico = {
    .verts = s_ico_verts, .edges = s_ico_edges,
    .n_edges = sizeof(s_ico_edges) / (sizeof(uint16_t) * 2),
};

/* ---- Dodecahedron (20 verts, 30 edges) ---- */
#define IPH 0.6180339887f
static const vec3_t s_dod_verts[20] = {
    { 1,  1,  1}, { 1,  1, -1}, { 1, -1,  1}, { 1, -1, -1},
    {-1,  1,  1}, {-1,  1, -1}, {-1, -1,  1}, {-1, -1, -1},
    { 0,  IPH,  PHI}, { 0,  IPH, -PHI}, { 0, -IPH,  PHI}, { 0, -IPH, -PHI},
    { IPH,  PHI, 0}, { IPH, -PHI, 0}, {-IPH,  PHI, 0}, {-IPH, -PHI, 0},
    { PHI, 0,  IPH}, { PHI, 0, -IPH}, {-PHI, 0,  IPH}, {-PHI, 0, -IPH},
};
static const uint16_t s_dod_edges[] = {
    0, 8,   0, 12,  0, 16,
    1, 9,   1, 12,  1, 17,
    2, 10,  2, 13,  2, 16,
    3, 11,  3, 13,  3, 17,
    4, 8,   4, 14,  4, 18,
    5, 9,   5, 14,  5, 19,
    6, 10,  6, 15,  6, 18,
    7, 11,  7, 15,  7, 19,
    8, 10,  9, 11,
    12, 14, 13, 15,
    16, 17, 18, 19,
};
static const model_t s_dod = {
    .verts = s_dod_verts, .edges = s_dod_edges,
    .n_edges = sizeof(s_dod_edges) / (sizeof(uint16_t) * 2),
};

/* ---- X-Wing (stylized) — fuselage + 4 wings + cockpit + laser tips ---- */
static vec3_t s_xwing_verts[20];
static uint16_t s_xwing_edges[64];
static model_t s_xwing;
static bool   s_xwing_built;

static void build_xwing(float foil_open)
{
    /* foil_open: 0 = stowed (||), 1 = locked-S-foils (X). */
    const float fl = 1.4f, fw = 0.18f, fh = 0.18f;
    vec3_t fus[8] = {
        {-fl, -fh, -fw}, { fl, -fh, -fw}, { fl,  fh, -fw}, {-fl,  fh, -fw},
        {-fl, -fh,  fw}, { fl, -fh,  fw}, { fl,  fh,  fw}, {-fl,  fh,  fw},
    };
    for (int i = 0; i < 8; ++i) s_xwing_verts[i] = fus[i];
    s_xwing_verts[8]  = (vec3_t){ fl - 0.1f,  fh + 0.18f, 0 };  /* cockpit bump */

    float spread = 1.6f;
    float lift   = 0.7f * foil_open;
    s_xwing_verts[9]  = (vec3_t){-fl + 0.1f,  fh, 0 };
    s_xwing_verts[10] = (vec3_t){-fl + 0.1f, -fh, 0 };
    s_xwing_verts[11] = (vec3_t){-fl - 0.6f,  lift,  spread };
    s_xwing_verts[12] = (vec3_t){-fl - 0.6f, -lift,  spread };
    s_xwing_verts[13] = (vec3_t){-fl - 0.6f,  lift, -spread };
    s_xwing_verts[14] = (vec3_t){-fl - 0.6f, -lift, -spread };
    s_xwing_verts[15] = (vec3_t){-fl - 0.2f,  lift,  spread };
    s_xwing_verts[16] = (vec3_t){-fl - 0.2f, -lift,  spread };
    s_xwing_verts[17] = (vec3_t){-fl - 0.2f,  lift, -spread };
    s_xwing_verts[18] = (vec3_t){-fl - 0.2f, -lift, -spread };

    int e = 0;
    uint16_t fus_e[] = { 0,1,1,2,2,3,3,0, 4,5,5,6,6,7,7,4, 0,4,1,5,2,6,3,7 };
    for (size_t i = 0; i < sizeof(fus_e)/sizeof(fus_e[0]); ++i) s_xwing_edges[e++] = fus_e[i];
    s_xwing_edges[e++] = 8; s_xwing_edges[e++] = 1;
    s_xwing_edges[e++] = 8; s_xwing_edges[e++] = 2;
    s_xwing_edges[e++] = 8; s_xwing_edges[e++] = 5;
    s_xwing_edges[e++] = 8; s_xwing_edges[e++] = 6;
    uint16_t wings[] = {
         9,11,  9,13,  10,12, 10,14,
        11,15, 12,16, 13,17, 14,18,
    };
    for (size_t i = 0; i < sizeof(wings)/sizeof(wings[0]); ++i) s_xwing_edges[e++] = wings[i];

    s_xwing.verts = s_xwing_verts;
    s_xwing.edges = s_xwing_edges;
    s_xwing.n_edges = e / 2;
    s_xwing_built = true;
}

/* ---- TIE Fighter — cockpit ball + two hex panels ---- */
static vec3_t s_tie_verts[20];
static uint16_t s_tie_edges[64];
static model_t s_tie;
static bool   s_tie_built;

static void build_tie(void)
{
    s_tie_verts[0] = (vec3_t){ 0.35f, 0, 0 };
    s_tie_verts[1] = (vec3_t){-0.35f, 0, 0 };
    s_tie_verts[2] = (vec3_t){ 0,  0.35f, 0 };
    s_tie_verts[3] = (vec3_t){ 0, -0.35f, 0 };
    s_tie_verts[4] = (vec3_t){ 0, 0,  0.35f };
    s_tie_verts[5] = (vec3_t){ 0, 0, -0.35f };
    for (int p = 0; p < 2; ++p) {
        float side = (p == 0) ? -1.4f : 1.4f;
        for (int i = 0; i < 6; ++i) {
            float a = (float)i * 2.0f * (float)M_PI / 6.0f;
            s_tie_verts[6 + p * 6 + i] = (vec3_t){ side, sinf(a) * 0.9f, cosf(a) * 0.9f };
        }
    }
    int e = 0;
    uint16_t coc[] = { 0,2, 0,3, 0,4, 0,5, 1,2, 1,3, 1,4, 1,5, 2,4, 4,3, 3,5, 5,2 };
    for (size_t i = 0; i < sizeof(coc)/sizeof(coc[0]); ++i) s_tie_edges[e++] = coc[i];
    for (int p = 0; p < 2; ++p) {
        int base = 6 + p * 6;
        for (int i = 0; i < 6; ++i) {
            s_tie_edges[e++] = (uint16_t)(base + i);
            s_tie_edges[e++] = (uint16_t)(base + (i + 1) % 6);
        }
        s_tie_edges[e++] = (uint16_t)(p == 0 ? 1 : 0);
        s_tie_edges[e++] = (uint16_t)base;
    }
    s_tie.verts = s_tie_verts;
    s_tie.edges = s_tie_edges;
    s_tie.n_edges = e / 2;
    s_tie_built = true;
}

/* ---- Death Star (lat/long sphere wireframe) ---- */
#define LAT 5
#define LON 10
#define DS_VERTS (2 + LAT * LON)
static vec3_t s_ds_verts[DS_VERTS];
static uint16_t s_ds_edges[(LON * 2 + LAT * LON + (LAT - 1) * LON) * 2];
static model_t s_ds;
static bool   s_ds_built;

static void build_deathstar(void)
{
    s_ds_verts[0] = (vec3_t){0, 1, 0};
    s_ds_verts[1] = (vec3_t){0, -1, 0};
    int v = 2;
    for (int i = 0; i < LAT; ++i) {
        float lat_a = ((float)(i + 1) / (LAT + 1) - 0.5f) * (float)M_PI;
        float y = sinf(lat_a);
        float r = cosf(lat_a);
        for (int j = 0; j < LON; ++j) {
            float lon_a = (float)j * 2.0f * (float)M_PI / LON;
            s_ds_verts[v++] = (vec3_t){ r * cosf(lon_a), y, r * sinf(lon_a) };
        }
    }
    int e = 0;
    for (int j = 0; j < LON; ++j) {
        s_ds_edges[e++] = 0;
        s_ds_edges[e++] = (uint16_t)(2 + j);
    }
    int last_ring = 2 + (LAT - 1) * LON;
    for (int j = 0; j < LON; ++j) {
        s_ds_edges[e++] = 1;
        s_ds_edges[e++] = (uint16_t)(last_ring + j);
    }
    for (int i = 0; i < LAT; ++i) {
        int base = 2 + i * LON;
        for (int j = 0; j < LON; ++j) {
            s_ds_edges[e++] = (uint16_t)(base + j);
            s_ds_edges[e++] = (uint16_t)(base + (j + 1) % LON);
        }
    }
    for (int i = 0; i < LAT - 1; ++i) {
        int base = 2 + i * LON;
        for (int j = 0; j < LON; ++j) {
            s_ds_edges[e++] = (uint16_t)(base + j);
            s_ds_edges[e++] = (uint16_t)(base + LON + j);
        }
    }
    s_ds.verts = s_ds_verts;
    s_ds.edges = s_ds_edges;
    s_ds.n_edges = e / 2;
    s_ds_built = true;
}

/* ---- Render ---- */

static mat4_t s_proj;
static bool   s_proj_ready;

static void label_at(fb_t *fb, const char *s, int y, uint16_t color)
{
    int w = 0; while (s[w]) w++;
    int x = (FB_W - w * 6) / 2;
    gfx_text_5x7(fb, x + 1, y + 1, s, fb_rgb565(0, 0, 0));
    gfx_text_5x7(fb, x,     y,     s, color);
}

/* Fade the rendered framebuffer FROM white toward its actual content
 * over the first `fade_ms` of the scene. Lets us start in white when the
 * previous scene ended in white (HYPERSPACE diegetic transition). */
static void fade_from_white(fb_t *fb, uint32_t t, uint32_t fade_ms)
{
    if (t >= fade_ms) return;
    /* k = 1 at t=0 (full white), k = 0 at t=fade_ms (no change). */
    int k_q = (int)((float)(fade_ms - t) / fade_ms * 32.0f);
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

static void render(void *ctx, fb_t *fb, uint32_t t)
{
    (void)ctx;
    if (!s_proj_ready) {
        s_proj = mat4_perspective((float)M_PI / 3.0f,
                                  (float)FB_W / FB_H, 0.1f, 100.0f);
        s_proj_ready = true;
    }

    gfx_clear(fb, fb_rgb565(0, 0, 16));
    float rt = t * 0.001f;
    mat4_t base = mat4_translation(0, 0, -5);

    if (t < 5000) {
        mat4_t world = mat4_mul(base, mat4_rotation_y(rt));
        world = mat4_mul(world, mat4_rotation_x(rt * 0.7f));
        gfx3d_draw_wireframe(fb, &s_cube, world, s_proj, fb_rgb565(0, 255, 255));
        label_at(fb, "CUBE", FB_H - 12, fb_rgb565(120, 220, 220));
    } else if (t < 10000) {
        if (!s_cyl_ready) build_cylinder();
        mat4_t world = mat4_mul(base, mat4_rotation_y(rt));
        world = mat4_mul(world, mat4_rotation_x(0.4f));
        gfx3d_draw_wireframe(fb, &s_cyl, world, s_proj, fb_rgb565(120, 255, 120));
        label_at(fb, "CYLINDER", FB_H - 12, fb_rgb565(120, 220, 120));
    } else if (t < 14000) {
        mat4_t world = mat4_mul(base, mat4_rotation_y(rt));
        world = mat4_mul(world, mat4_rotation_z(rt * 0.5f));
        gfx3d_draw_wireframe(fb, &s_octa, world, s_proj, fb_rgb565(255, 200, 80));
        label_at(fb, "OCTAHEDRON", FB_H - 12, fb_rgb565(255, 220, 120));
    } else if (t < 18000) {
        mat4_t world = mat4_translation(0, 0, -7);
        world = mat4_mul(world, mat4_rotation_y(rt));
        world = mat4_mul(world, mat4_rotation_x(rt * 0.6f));
        gfx3d_draw_wireframe(fb, &s_ico, world, s_proj, fb_rgb565(180, 180, 255));
        label_at(fb, "ICOSAHEDRON", FB_H - 12, fb_rgb565(200, 200, 255));
    } else if (t < 23000) {
        mat4_t world = mat4_translation(0, 0, -7);
        world = mat4_mul(world, mat4_rotation_y(rt * 0.7f));
        world = mat4_mul(world, mat4_rotation_x(rt * 0.4f));
        gfx3d_draw_wireframe(fb, &s_dod, world, s_proj, fb_rgb565(255, 140, 220));
        label_at(fb, "DODECAHEDRON", FB_H - 12, fb_rgb565(255, 180, 230));
    } else if (t < 30000) {
        /* X-Wing solo. S-foils split open across t = 25000..28000. */
        float open;
        if (t < 25000) open = 0.0f;
        else if (t > 28000) open = 1.0f;
        else open = (float)(t - 25000) / 3000.0f;
        build_xwing(open);
        mat4_t world = mat4_translation(0, 0, -5);
        world = mat4_mul(world, mat4_rotation_y(rt * 0.4f));
        gfx3d_draw_wireframe(fb, &s_xwing, world, s_proj, fb_rgb565(220, 220, 255));
        label_at(fb,
                 (t < 25000) ? "X-WING" :
                 (t < 28000) ? "S-FOILS LOCKED" : "X-WING",
                 FB_H - 12, fb_rgb565(220, 220, 255));
    } else if (t < 37000) {
        /* X-Wing + TIE dogfight. */
        if (!s_tie_built) build_tie();
        build_xwing(1.0f);
        float dt = (t - 30000) * 0.001f;

        mat4_t xworld = mat4_translation(-1.6f, 0.3f * sinf(dt * 1.3f), -6.0f);
        xworld = mat4_mul(xworld, mat4_rotation_y((float)M_PI * 0.15f + sinf(dt) * 0.15f));
        xworld = mat4_mul(xworld, mat4_rotation_z(sinf(dt * 1.5f) * 0.3f));
        gfx3d_draw_wireframe(fb, &s_xwing, xworld, s_proj, fb_rgb565(220, 220, 255));

        mat4_t tworld = mat4_translation(1.8f, -0.4f * sinf(dt * 1.1f), -6.0f);
        tworld = mat4_mul(tworld, mat4_rotation_y(-(float)M_PI * 0.15f + cosf(dt) * 0.15f));
        gfx3d_draw_wireframe(fb, &s_tie, tworld, s_proj, fb_rgb565(255, 100, 100));

        /* Laser shots: alternating beats of red and green vector lines. */
        float beat = dt - floorf(dt / 1.2f) * 1.2f;
        if (beat < 0.25f) {
            int sx, sy, ex, ey;
            int ok1 = gfx3d_project((vec3_t){-1.0f, 0, -5.0f}, s_proj, &sx, &sy);
            int ok2 = gfx3d_project((vec3_t){ 2.0f, 0, -5.0f}, s_proj, &ex, &ey);
            if (ok1 && ok2) gfx_line(fb, sx, sy, ex, ey, fb_rgb565(255, 0, 60));
        }
        float beat2 = (dt + 0.6f) - floorf((dt + 0.6f) / 1.2f) * 1.2f;
        if (beat2 < 0.25f) {
            int sx, sy, ex, ey;
            int ok1 = gfx3d_project((vec3_t){ 1.0f, 0, -5.0f}, s_proj, &sx, &sy);
            int ok2 = gfx3d_project((vec3_t){-2.0f, 0, -5.0f}, s_proj, &ex, &ey);
            if (ok1 && ok2) gfx_line(fb, sx, sy, ex, ey, fb_rgb565(0, 255, 80));
        }
        label_at(fb, "DOGFIGHT", FB_H - 12, fb_rgb565(255, 200, 200));
    } else {
        /* Pullback + Death Star reveal. */
        if (!s_ds_built) build_deathstar();
        if (!s_xwing_built) build_xwing(1.0f);
        if (!s_tie_built) build_tie();
        float dt = (t - 37000) * 0.001f;
        float pull = dt / 8.0f;
        if (pull > 1.0f) pull = 1.0f;

        /* Death Star drifts in from upper right, distant. */
        mat4_t ds_world = mat4_translation(2.0f + pull * 0.5f, 0.6f, -12.0f - pull * 4.0f);
        ds_world = mat4_mul(ds_world, mat4_rotation_y(dt * 0.15f));
        gfx3d_draw_wireframe(fb, &s_ds, ds_world, s_proj, fb_rgb565(180, 180, 200));

        /* X-Wing flies toward the Death Star (sweeps from left to upper-right). */
        mat4_t xworld = mat4_translation(-3.0f + pull * 4.0f, 0.5f - pull * 0.3f, -8.0f - pull * 2.0f);
        xworld = mat4_mul(xworld, mat4_rotation_y((float)M_PI * 0.12f));
        gfx3d_draw_wireframe(fb, &s_xwing, xworld, s_proj, fb_rgb565(220, 220, 255));

        /* TIE follows briefly then peels off. */
        if (pull < 0.7f) {
            mat4_t tworld = mat4_translation(-4.5f + pull * 4.0f, -0.6f, -8.0f - pull * 2.0f);
            tworld = mat4_mul(tworld, mat4_rotation_y(-(float)M_PI * 0.12f));
            gfx3d_draw_wireframe(fb, &s_tie, tworld, s_proj, fb_rgb565(255, 100, 100));
        }

        if (pull > 0.4f) {
            uint8_t bright = (uint8_t)(180 * (pull - 0.4f) / 0.6f);
            label_at(fb, "TO BE CONTINUED ...", 8,
                     fb_rgb565(bright, bright, (uint8_t)(bright + 40 > 255 ? 255 : bright + 40)));
        }
    }

    /* HYPERSPACE diegetic transition: fade from white in the first 500ms
     * so the white flash that ended Scene 03 visually delivers us here. */
    fade_from_white(fb, t, 500);
}

const scene_t SCENE_04_CUBE = {
    .name = "vector_parade",
    .est_duration_ms = SCENE_DURATION_MS,
    .render = render,
};

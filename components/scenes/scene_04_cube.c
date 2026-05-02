#include "scenes.h"
#include "gfx.h"
#include "gfx3d.h"
#include <math.h>
#include <stdbool.h>

static const vec3_t s_cube_verts[8] = {
    {-1,-1,-1}, { 1,-1,-1}, { 1, 1,-1}, {-1, 1,-1},
    {-1,-1, 1}, { 1,-1, 1}, { 1, 1, 1}, {-1, 1, 1},
};

static const uint16_t s_cube_edges[] = {
    0,1, 1,2, 2,3, 3,0,         /* back face */
    4,5, 5,6, 6,7, 7,4,         /* front face */
    0,4, 1,5, 2,6, 3,7,         /* connecting */
};

static const model_t s_cube = {
    .verts = s_cube_verts, .edges = s_cube_edges,
    .n_edges = sizeof(s_cube_edges) / (sizeof(uint16_t) * 2),
};

static void render(void *ctx, fb_t *fb, uint32_t t)
{
    (void)ctx;
    gfx_clear(fb, fb_rgb565(0, 0, 16));

    float rt = t / 1000.0f;
    mat4_t world = mat4_translation(0, 0, -5);
    world = mat4_mul(world, mat4_rotation_y(rt));
    world = mat4_mul(world, mat4_rotation_x(rt * 0.7f));

    /* Perspective is constant — build once on first frame, cache. */
    static mat4_t s_proj;
    static bool   s_proj_ready;
    if (!s_proj_ready) {
        s_proj = mat4_perspective((float)M_PI / 3.0f,
                                  (float)FB_W / FB_H, 0.1f, 100.0f);
        s_proj_ready = true;
    }

    gfx3d_draw_wireframe(fb, &s_cube, world, s_proj, fb_rgb565(0, 255, 255));
}

const scene_t SCENE_04_CUBE = {
    .name = "cube",
    .est_duration_ms = 8000,
    .render = render,
};

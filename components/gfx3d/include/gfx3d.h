#pragma once
#include <stdint.h>
#include "fb.h"

typedef struct { float x, y, z; } vec3_t;
typedef struct { float m[16]; }   mat4_t;          /* column-major */

mat4_t mat4_identity   (void);
mat4_t mat4_mul        (mat4_t a, mat4_t b);
mat4_t mat4_rotation_x (float radians);
mat4_t mat4_rotation_y (float radians);
mat4_t mat4_rotation_z (float radians);
mat4_t mat4_translation(float tx, float ty, float tz);
mat4_t mat4_perspective(float fov_y_radians, float aspect, float near_, float far_);

vec3_t mat4_transform_point(mat4_t m, vec3_t v);

/* Project a transformed point into framebuffer coords. Returns 0 if behind
 * the near plane (caller should not draw). x/y are written on success. */
int gfx3d_project(vec3_t v_view, mat4_t proj, int *out_x, int *out_y);

/* Render a wireframe model: vertex array + edge index pairs. */
typedef struct {
    const vec3_t  *verts;
    const uint16_t *edges;   /* pairs of indices: [a0,b0, a1,b1, ...] */
    int             n_edges;
} model_t;

void gfx3d_draw_wireframe(fb_t *fb, const model_t *m, mat4_t world,
                          mat4_t proj, uint16_t color);

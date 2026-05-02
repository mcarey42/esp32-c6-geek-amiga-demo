#include "gfx3d.h"
#include "gfx.h"
#include <math.h>
#include <string.h>

/* Column-major 4x4. Element (row r, col c) is m[c*4 + r]. */
#define M(mat, r, c) ((mat).m[(c)*4 + (r)])

mat4_t mat4_identity(void)
{
    mat4_t r = {{0}};
    for (int i = 0; i < 4; ++i) M(r, i, i) = 1.0f;
    return r;
}

mat4_t mat4_mul(mat4_t a, mat4_t b)
{
    mat4_t r;
    for (int c = 0; c < 4; ++c)
        for (int rr = 0; rr < 4; ++rr) {
            float s = 0;
            for (int k = 0; k < 4; ++k) s += M(a, rr, k) * M(b, k, c);
            M(r, rr, c) = s;
        }
    return r;
}

mat4_t mat4_rotation_x(float t)
{
    float c = cosf(t), s = sinf(t);
    mat4_t r = mat4_identity();
    M(r, 1, 1) =  c; M(r, 1, 2) = -s;
    M(r, 2, 1) =  s; M(r, 2, 2) =  c;
    return r;
}

mat4_t mat4_rotation_y(float t)
{
    float c = cosf(t), s = sinf(t);
    mat4_t r = mat4_identity();
    M(r, 0, 0) =  c; M(r, 0, 2) =  s;
    M(r, 2, 0) = -s; M(r, 2, 2) =  c;
    return r;
}

mat4_t mat4_rotation_z(float t)
{
    float c = cosf(t), s = sinf(t);
    mat4_t r = mat4_identity();
    M(r, 0, 0) =  c; M(r, 0, 1) = -s;
    M(r, 1, 0) =  s; M(r, 1, 1) =  c;
    return r;
}

mat4_t mat4_translation(float tx, float ty, float tz)
{
    mat4_t r = mat4_identity();
    M(r, 0, 3) = tx; M(r, 1, 3) = ty; M(r, 2, 3) = tz;
    return r;
}

mat4_t mat4_perspective(float fov_y, float aspect, float n, float f)
{
    float t = 1.0f / tanf(fov_y * 0.5f);
    mat4_t r = {{0}};
    M(r, 0, 0) = t / aspect;
    M(r, 1, 1) = t;
    M(r, 2, 2) = (f + n) / (n - f);
    M(r, 2, 3) = (2.0f * f * n) / (n - f);
    M(r, 3, 2) = -1.0f;
    return r;
}

vec3_t mat4_transform_point(mat4_t m, vec3_t v)
{
    float x = M(m,0,0)*v.x + M(m,0,1)*v.y + M(m,0,2)*v.z + M(m,0,3);
    float y = M(m,1,0)*v.x + M(m,1,1)*v.y + M(m,1,2)*v.z + M(m,1,3);
    float z = M(m,2,0)*v.x + M(m,2,1)*v.y + M(m,2,2)*v.z + M(m,2,3);
    return (vec3_t){x, y, z};
}

int gfx3d_project(vec3_t v_view, mat4_t proj, int *out_x, int *out_y)
{
    /* v_view is camera-space (looking down -Z). Reject if z >= -near. */
    if (v_view.z >= -0.05f) return 0;

    float x = M(proj,0,0)*v_view.x + M(proj,0,2)*v_view.z;
    float y = M(proj,1,1)*v_view.y + M(proj,1,2)*v_view.z;
    float w = M(proj,3,2)*v_view.z;
    if (w == 0) return 0;
    float ndc_x = x / w;
    float ndc_y = y / w;
    *out_x = (int)((ndc_x * 0.5f + 0.5f) * FB_W);
    *out_y = (int)((1.0f - (ndc_y * 0.5f + 0.5f)) * FB_H);
    return 1;
}

void gfx3d_draw_wireframe(fb_t *fb, const model_t *m, mat4_t world,
                          mat4_t proj, uint16_t color)
{
    for (int e = 0; e < m->n_edges; ++e) {
        uint16_t a = m->edges[e * 2 + 0];
        uint16_t b = m->edges[e * 2 + 1];
        vec3_t va = mat4_transform_point(world, m->verts[a]);
        vec3_t vb = mat4_transform_point(world, m->verts[b]);
        int xa, ya, xb, yb;
        if (gfx3d_project(va, proj, &xa, &ya) &&
            gfx3d_project(vb, proj, &xb, &yb)) {
            gfx_line(fb, xa, ya, xb, yb, color);
        }
    }
}
